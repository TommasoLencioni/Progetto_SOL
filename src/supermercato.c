#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <cassiere.h>
#include <cliente.h>
#include <linkedlist.h>
#include <getopt.h>
#include <supermercato.h>
#ifndef DEBUG
#define DEBUG 0
#endif

//MUTEX/////////////////////////////////////////////////////////////////////////

static pthread_mutex_t mtxcasse = PTHREAD_MUTEX_INITIALIZER;    //MUTEX PER L'UTILIZZO DEL NUMERO DI CASSE APERTE
pthread_mutex_t mtxclienti= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t noclienti = PTHREAD_COND_INITIALIZER;     //VARIABILE DI CONDIZIONE DA SEGNALARE IN CASO DI SUPERMERCATO SENZA CLIENTI
static pthread_mutex_t mtxnoprod = PTHREAD_MUTEX_INITIALIZER;   //MUTEX CODA CLIENTI SENZA PRODOTTI
static pthread_cond_t emptynoprod = PTHREAD_COND_INITIALIZER;   //VARIABILE DI CONDIZIONE CODA CLIENTI SENZA PRODOTTI VUOTA
static pthread_mutex_t mtxlog = PTHREAD_MUTEX_INITIALIZER;      //MUTEX PER LA SCRITTURA CONCORRENTE SUL FILE DI LOG

//SEGNALI///////////////////////////////////////////////////////////////////////

//FLAGS DA SETTARE NEGLI HANDLER DEI SEGNALI
volatile sig_atomic_t sighup_flag = 0;
volatile sig_atomic_t sigquit_flag = 0;

//HANDLER SIGHUP
void gestore_hup (int sig){
    write(1,"Ricevuto SIGHUP, stop ingressi\n",31);
    sighup_flag = 1;
}

//HANDLER SIGQUIT
void gestore_quit (int sig){
    write(1,"Ricevuto SIGQUIT, stop ingressi\n",32);
    sigquit_flag = 1;
    sighup_flag = 1;
}

//MISCELLANEOUS/////////////////////////////////////////////////////////////////

//FUNIONE GENERICA DI ATTESA UTILIZZATA DAL DIRETTORE E DALLE CASSE
void attesa (int amount){
    struct timespec tempo;
    tempo.tv_sec=0;
    tempo.tv_nsec = amount*1000000;
    while (nanosleep(&tempo, &tempo));
    return;
}

//COMUNICAZIONE CASSA-DIRETTORE/////////////////////////////////////////////////

void * gestore_comunicazione (void * arg){
    pthread_detach(pthread_self());
    struct struct_cassiere * argomenti=arg;
    int inf;
    while(!sighup_flag){
        CHECK_ERR(pthread_mutex_lock(argomenti->coda_mutex),"Acquisizione lock clienti in coda");
        inf=(argomenti->in_coda)->count;
        CHECK_ERR(pthread_mutex_unlock(argomenti->coda_mutex),"Rilascio lock clienti in coda");
        CHECK_ERR(write(argomenti->pfd[1],&inf,sizeof(int)),"Scrittura informazione su pipe");
        attesa(argomenti->costanti->I);
    }
    pthread_exit(NULL);
}

//CASSIERE//////////////////////////////////////////////////////////////////////

void * fun_cassiere (void * arg){
    struct struct_cassiere * argomenti=arg;
    pthread_cleanup_push(cassiere_cleanup,argomenti);
    cassiere_initialize(&argomenti);
    pthread_t * comunicazione=malloc(sizeof(pthread_t));
    CHECK_PTR(comunicazione,"Allocazione thread comunicazione");
    if((pthread_create(comunicazione, NULL, gestore_comunicazione, argomenti))!=0){
        fprintf(stderr, "Creazione thread comunicazione fallita\n");
        exit(EXIT_FAILURE);
    }
    FILE * log;
    float t_medio=0;    //Variabile per il calcolo del tempo medio
    while(1){
        CHECK_ERR(pthread_mutex_lock(argomenti->self),"Acquisizione lock self");
        while(!argomenti->status) CHECK_ERR(pthread_cond_wait(argomenti->empty,argomenti->self),"Wait per coda vuota");
        CHECK_ERR(pthread_mutex_unlock(argomenti->self),"Rilascio lock self");
        CHECK_ERR(pthread_mutex_lock(&mtxcasse),"Acquisizione lock casse");
        argomenti->costanti->K_open++;
        CHECK_ERR(pthread_mutex_unlock(&mtxcasse),"Rilascio lock casse");
        if(DEBUG)printf("K %d aperta\n",argomenti->id);
        while(argomenti->status){
                CHECK_ERR(pthread_mutex_lock(argomenti->self),"Acquisizione lock");
                if(argomenti->status==1){
                    CHECK_ERR(pthread_mutex_unlock(argomenti->self),"Rilascio lock");
                    if(argomenti->ordine_chiusura){
                        CHECK_ERR(pthread_mutex_lock(argomenti->self),"Acquisizione lock");
                        argomenti->status=0;
                        if(argomenti->serviti) t_medio=(float)(((argomenti->tempo_apertura)/argomenti->serviti)-argomenti->delay)/1000;
                        log = fopen(argomenti->costanti->LOG, "a");
                        CHECK_PTR(log,"Apertura file di log");
                        CHECK_ERR(pthread_mutex_lock(&mtxlog),"Acquisizione mutex log");
                        fprintf(log,"K|%d|%d|%d|%.3f|%.3f|%d\n",argomenti->id, argomenti->prod_elaborati, argomenti->serviti, ((float)argomenti->tempo_apertura)/1000, t_medio, argomenti->chiusure);
                        CHECK_ERR(pthread_mutex_unlock(&mtxlog),"Rilascio mutex log");
                        CHECK_ERR(pthread_mutex_unlock(argomenti->self),"Rilascio lock");
                        CHECK_ERR(fclose(log),"Chiusura file di log");
                        close(argomenti->pfd[0]);
                        close(argomenti->pfd[1]);
                        pthread_exit(NULL);
                    }
                    if(!sigquit_flag) {
                            serve(&argomenti,pop_head(&(argomenti->in_coda),argomenti->coda_mutex,argomenti->empty));
                            CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
                            if(!(argomenti->ordine_chiusura))(argomenti->costanti)->C_in--;
                            if((argomenti->costanti)->C_in==0) CHECK_ERR(pthread_cond_signal(&noclienti),"Signal no clienti");
                            CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
                    }
                    else{
                        fa_uscire(&argomenti,pop_head(&(argomenti->in_coda),argomenti->coda_mutex,argomenti->empty));
                        CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
                        if((argomenti->costanti)->C_in==0) CHECK_ERR(pthread_cond_signal(&noclienti),"Signal no clienti");
                        CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
                    }
                }
                CHECK_ERR(pthread_mutex_unlock(argomenti->self),"Rilascio lock");
        }
    }
    pthread_cleanup_pop(1);
    return NULL;
}


//CLIENTE///////////////////////////////////////////////////////////////////////

void * fun_cliente (void * arg){
    pthread_detach(pthread_self());
    struct struct_cliente * argomenti=arg;
    pthread_cleanup_push(cliente_cleanup, argomenti);
    cliente_initilize(&argomenti);
    FILE * log;
    log = fopen(argomenti->costanti->LOG, "a");
    CHECK_PTR(log,"Apertura file di log");
    attesa(argomenti->delay);
    if(argomenti->prodotti==0){
        CHECK_ERR(pthread_mutex_lock(argomenti->cliente_mutex),"Acquisizione lock cliente");
        insert_tail(argomenti, &(argomenti->noprod) ,&mtxnoprod,&emptynoprod);
        while(argomenti->prodotti==0) CHECK_ERR(pthread_cond_wait(argomenti->permesso_uscita,argomenti->cliente_mutex),"Wait no-prodotti");
        CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
        (argomenti->costanti)->C_in--;
        if((argomenti->costanti)->C_in==0) CHECK_ERR(pthread_cond_signal(&noclienti),"Signal no clienti");
        CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
        if(DEBUG) printf("C %d uscito con 0 prodotti\n", argomenti->id);
        argomenti->prodotti++;
        CHECK_ERR(pthread_mutex_unlock(argomenti->cliente_mutex),"Rilascio lock");
        CHECK_ERR(pthread_mutex_lock(&mtxlog),"Acquisizione mutex log");
        fprintf(log,"C|%d|%d|%.3f|%.3f|%d\n",argomenti->id, argomenti->prodotti, (float)(argomenti->tempotot)/1000, (float)(argomenti->tempotot)/1000, argomenti->code_visitate);
        CHECK_ERR(pthread_mutex_unlock(&mtxlog),"Rilascio mutex log");
        CHECK_ERR(fclose(log),"Chiusura file di log");
        return NULL;
    }
    scelta_coda(&argomenti);
    if(argomenti->quit) fprintf(log,"CQ|%d|0|%.3f|%.3f|%d\n",argomenti->id, (float)(argomenti->tempotot+argomenti->delay)/1000, (float)(argomenti->tempotot)/1000, argomenti->code_visitate);
    else fprintf(log,"C|%d|%d|%.3f|%.3f|%d\n",argomenti->id, argomenti->prodotti,  (float)(argomenti->tempotot+argomenti->delay)/1000, (float)(argomenti->tempotot)/1000, argomenti->code_visitate);
    CHECK_ERR(fclose(log),"Chiusura file di log");
    pthread_cleanup_pop(1);
    return NULL;
}


//CLIENTE FITTIZIO//////////////////////////////////////////////////////////////

void * fun_clientefittizio (void * arg){
    pthread_detach(pthread_self());
    struct struct_cliente * argomenti=arg;
    pthread_cleanup_push(clientefittizio_cleanup, argomenti);
    clientefittizio_initilize(&argomenti);
    avvisa_cassiere(&argomenti);
    pthread_cleanup_pop(1);
    return NULL;
}


//INGRESSI//////////////////////////////////////////////////////////////////////
void * fun_ingressi(void * arg){
    struct struct_ingressi * argomenti=arg;
    int attuale;
    while(!sighup_flag){
        CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
        attuale=(argomenti->costanti->C_in);
        CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
        if(attuale<((argomenti->costanti->C)-(argomenti->costanti->E))){
            for(int i=0; i<(argomenti->costanti->E); i++){
                struct struct_cliente * tempcliente=malloc(sizeof(struct struct_cliente));
                tempcliente->id=(argomenti->costanti->C_id);
                tempcliente->casse=argomenti->casse;
                tempcliente->costanti=argomenti->costanti;
                tempcliente->noprod=argomenti->noprod;
                if((pthread_create(&(tempcliente->thread), NULL, fun_cliente, &(*tempcliente)))!=0){
                    fprintf(stderr, "Creazione thread cliente fallita\n");
                    exit(EXIT_FAILURE);
                }
                CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
                (argomenti->costanti->C_id)++;
                (argomenti->costanti->C_in)++;
                CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
            }
        }
    }
    pthread_exit(NULL);
}


//NO PROD///////////////////////////////////////////////////////////////////////
void * fun_noprod(void * arg){
    pthread_detach(pthread_self());
    linkedlist * lista= arg;
    while(1){
        struct node * primo=pop_head(&lista,&mtxnoprod,&emptynoprod);
        struct struct_cliente * cliente = (*primo).data;
        CHECK_ERR(pthread_mutex_lock((*cliente).cliente_mutex),"Acquisizione lock");
        (*cliente).prodotti=-1;     //evito wake up spurie nel ciclo dove il cliente va in attesa del permesso, il valore tornerà a 0 per una corretta scrittura sul file di log
        CHECK_ERR(pthread_cond_signal((*cliente).permesso_uscita),"Permesso uscita dal direttore");
        CHECK_ERR(pthread_mutex_unlock((*cliente).cliente_mutex),"Rilascio lock");
    }
}


//DIRETTORE/////////////////////////////////////////////////////////////////////
void * fun_direttore (void * arg){
    struct struct_costanti * costanti=arg;
    int s1flag=0;   //Contatore per il raggiungimento della soglia S1
    int s2flag=0; //Contatore per il raggiungimento della soglia S2
    int Cmin; //minor numero di clienti
    int id_Cmin;  //indice cassa col minor numero di clienti
    int * tmpk=malloc(sizeof(int));
    CHECK_PTR(tmpk,"Allocazione messaggio temporaneo");
    costanti->C_in=0;
    costanti->C_id=0;
    costanti->K_open=0;

    //GESTORI SEGNALI
    struct sigaction s1;
    memset(&s1,0,sizeof(s1));
    s1.sa_handler=gestore_hup;
    CHECK_ERR(sigaction(SIGHUP,&s1,NULL),"Nuovo handler SIGHUP");

    struct sigaction s2;
    memset(&s2,0,sizeof(s2));
    s2.sa_handler=gestore_quit;
    CHECK_ERR(sigaction(SIGQUIT,&s2,NULL),"Nuovo handler SIGQUIT");

    //CREAZIONE CASSIERI
    *tmpk=-1;
    struct struct_cassiere ** cassieri=malloc(sizeof(struct struct_cassiere *)*(costanti->K));
    CHECK_PTR(cassieri,"Allocazione cassieri");
    for(int i=0; i<(costanti->K);i++){
        cassieri[i]=malloc(sizeof(struct struct_cassiere));
        CHECK_PTR(cassieri[i], "Allocazione cassiere");
    }
    for(int i=0; i<(costanti->K); i++){
        (*cassieri[i]).id=i;
        (*cassieri[i]).status=0;
        (*cassieri[i]).costanti=costanti;
        pipe((*cassieri[i]).pfd);
        CHECK_ERR(pipe((*cassieri[i]).pfd),"Creazione pipe");
        CHECK_ERR(write(((*cassieri[i]).pfd)[1],tmpk,sizeof(int)),"Scrittura informazione su pipe");
        if((pthread_create(&((*cassieri[i]).thread), NULL, fun_cassiere, &(*cassieri[i])))!=0){
            fprintf(stderr, "Creazione thread cassiere fallita\n");
    	    exit(EXIT_FAILURE);
        }
    }

    //ATTENDO CHE TUTTE LE CASSE SIANO INIZIALIZZATE E COMUNICANTI
    int tmpcount;
    while(1){
        tmpcount=0;
        for(int i=0; i<(costanti->K); i++){
            CHECK_ERR(read((cassieri[i])->pfd[0],tmpk,sizeof(int)),"Lettura da pipe");
            if(*tmpk>=0) tmpcount++;
        }
        if(tmpcount==costanti->K) break;
    }

    //APERTURA PRIME K_ini CASSE
    for(int i=0; i<costanti->K_ini; i++){
        CHECK_ERR(pthread_mutex_lock((*cassieri[i]).self),"Acquisizione lock");
        (*cassieri[i]).status=1;
        CHECK_ERR(pthread_cond_signal((*cassieri[i]).empty),"Signal attivazione cassa");
        CHECK_ERR(pthread_mutex_unlock((*cassieri[i]).self),"Acquisizione lock");
    }

    //CREAZIONE GESTORE CLINTI SENZA PRODOTTI
    linkedlist * noprod=malloc(sizeof(linkedlist));
    CHECK_PTR(noprod,"Allocazione lista noprod");
    linkedlist_initializer(&noprod);
    pthread_t * gestore_noprod=malloc(sizeof(pthread_t));
    CHECK_PTR(gestore_noprod,"Allocazione thread gestre noprod");
    if((pthread_create(gestore_noprod, NULL, fun_noprod, noprod))!=0){
        fprintf(stderr, "Creazione thread gestione clienti senza prodotti fallita\n");
        exit(EXIT_FAILURE);
    }

    //CREAZIONE PRIMI C CLIENTI
    for(int i=0; i<(costanti->C); i++){
        struct struct_cliente * tempcliente=malloc(sizeof(struct struct_cliente));
        tempcliente->id=costanti->C_id;
        tempcliente->casse=cassieri;
        tempcliente->costanti=costanti;
        tempcliente->noprod=noprod;
        if((pthread_create(&(tempcliente->thread), NULL, fun_cliente, &(*tempcliente)))!=0){
            fprintf(stderr, "Creazione thread cliente fallita\n");
    	    exit(EXIT_FAILURE);
        }
        CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
        costanti->C_id++;
        costanti->C_in++;
        CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
    }

    //CREAZIONE GESTORE INGRESSI
    pthread_t * gestore_ingressi=malloc(sizeof(pthread_t));
    CHECK_PTR(gestore_ingressi,"Allocazione thread gestione ingressi");
    struct struct_ingressi * ingressi =malloc(sizeof(struct struct_ingressi));
    ingressi->costanti=costanti;
    ingressi->casse=cassieri;
    ingressi->noprod=noprod;
    if((pthread_create(gestore_ingressi, NULL, fun_ingressi, &(*ingressi)))!=0){
        fprintf(stderr, "Creazione thread gestione ingressi fallita\n");
        exit(EXIT_FAILURE);
    }

    //CICLO DI CONTROLLO DATI COMUNICATI DALLE CASSE E DECISIONI DEL DIRETTORE
    int * tmp_info=malloc(sizeof(int));
    CHECK_PTR(tmp_info,"Allocazione tmp info");
    while(!sighup_flag){
        s1flag=0;
        s2flag=0;
        Cmin=(costanti->C)+1;
        id_Cmin=0;
        for(int i=0; i<costanti->K; i++){
            CHECK_ERR(pthread_mutex_lock((cassieri[i])->self),"Acquisizione lock cassa");
            if((cassieri[i])->status==1){
                CHECK_ERR(read((cassieri[i])->pfd[0],tmp_info,sizeof(int)),"Lettura da pipe");
                if(DEBUG) printf("#### K %d, ho %d clienti in coda, ne ho serviti %d\n",i,*tmp_info, (cassieri[i])->serviti);
                if(*tmp_info<=Cmin){
                    Cmin=*tmp_info;
                    id_Cmin=i;
                }
                if(*tmp_info<=1) s1flag++;
                if(*tmp_info>=costanti->S2) s2flag++;
            }
            CHECK_ERR(pthread_mutex_unlock((cassieri[i])->self),"Rilascio lock cassa");
        }
        if(DEBUG){
            CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
            CHECK_ERR(pthread_mutex_lock(&mtxcasse),"Acquisizione lock casse");
            printf("s1 %d || s2 %d|| K_open %d || C_in %d\n", s1flag, s2flag, costanti->K_open, costanti->C_in);
            CHECK_ERR(pthread_mutex_unlock(&mtxcasse),"Rilascio lock casse");
            CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
        }
        if(!((s1flag>=(costanti->S1)) && ((costanti->K_open)>1)) || !(s2flag && ((costanti->K_open)<(costanti->K)))){
            if((s1flag>=(costanti->S1)) && ((costanti->K_open)>1)){
                CHECK_ERR(pthread_mutex_lock((*cassieri[id_Cmin]).self),"Acquisizione lock cassa");
                (*cassieri[id_Cmin]).status=0;
                (*cassieri[id_Cmin]).chiusure++;
                while(linkedlist_count(((*cassieri[id_Cmin]).in_coda),(*cassieri[id_Cmin]).coda_mutex)){
                    sposta(&cassieri[id_Cmin],pop_head(&((*cassieri[id_Cmin]).in_coda),(*cassieri[id_Cmin]).coda_mutex,(*cassieri[id_Cmin]).empty));
                }
                CHECK_ERR(pthread_mutex_unlock((*cassieri[id_Cmin]).self),"Rilascio lock");
                CHECK_ERR(pthread_mutex_lock(&mtxcasse),"Acquisizione lock casse");
                costanti->K_open--;
                CHECK_ERR(pthread_mutex_unlock(&mtxcasse),"Rilascio lock casse");
                CHECK_ERR(pthread_mutex_unlock((*cassieri[id_Cmin]).self),"Rilascio lock cassa");
                if(DEBUG) printf("## Ho chiuso la cassa %d\n", id_Cmin);
            }

            if(s2flag && ((costanti->K_open)<(costanti->K))){
                for(int i=0; i<(costanti->K); i++){
                    CHECK_ERR(pthread_mutex_lock((*cassieri[i]).self),"Acquisizione lock");
                    if((*cassieri[i]).status==0){
                        (*cassieri[i]).status=1;
                        CHECK_ERR(pthread_cond_signal((*cassieri[i]).empty),"Signal attivazione cassa");
                        CHECK_ERR(pthread_mutex_unlock((*cassieri[i]).self),"Acquisizione lock");
                        if(DEBUG) printf("## Ho aperto la cassa %d\n", i);
                        break;
                    }
                    CHECK_ERR(pthread_mutex_unlock((*cassieri[i]).self),"Rilascio lock");
                }
            }
        }
        attesa(costanti->I);
    }

    //ATTESA 0 CLIENTI IN
    CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
    while(costanti->C_in){
        CHECK_ERR(pthread_cond_wait(&noclienti,&mtxclienti),"Wait no clienti");
    };
    CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");

    //CREAZIONE CLIENTI FITTIZI
    for(int i=0; i<costanti->K; i++){
        CHECK_ERR(pthread_mutex_lock((*cassieri[i]).self),"Acquisizione lock");
        if(!(*cassieri[i]).status){
            (*cassieri[i]).status=1;
        }
        CHECK_ERR(pthread_mutex_unlock((*cassieri[i]).self),"Rilascio lock");
        struct struct_cliente * tempcliente=malloc(sizeof(struct struct_cliente));
        tempcliente->id=costanti->C_id;
        tempcliente->casse=cassieri;
        tempcliente->costanti=costanti;
        tempcliente->noprod=noprod;
        tempcliente->tokill=i;
        CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
        costanti->C_id++;
        CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
        if((pthread_create(&(tempcliente->thread), NULL, fun_clientefittizio, &(*tempcliente)))!=0){
            fprintf(stderr, "Creazione thread cliente fittizio fallita\n");
    	    exit(EXIT_FAILURE);
        }
    }

    //JOIN DELLE CASSE
    for(int i=0; i<costanti->K; i++){
        if (pthread_join(((*cassieri[i]).thread), NULL) == -1) {
            fprintf(stderr, "pthread_join failed\n");
        }
    }

    //CHIUSURA GESTORE_NOPROD
    CHECK_ERR(pthread_cancel(*gestore_noprod),"Chiusura gestore no-prodotti");

    //FREES
    free(cassieri);
    free(noprod);
    free(gestore_noprod);
    free(gestore_ingressi);
    free(ingressi);
    pthread_exit(NULL);
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////MAIN
int main(int argc, char * argv[]){
    printf("### SUPERMERCATO APERTO ###\n");

    //PARSING OPZIONI
    int logflag=0, opt;
    char * tmplog=malloc(sizeof(char)*BUFFERSIZE);
    while ((opt=getopt(argc, argv, "l:")) != -1)
    switch (opt) {
        case 'l':
            logflag = 1;
            printf("Given File: %s\n", optarg);
            strcpy(tmplog,optarg);
            break;
        case '?':
            printf("unknown option: %c\n", optopt);
            exit(EXIT_FAILURE);
    }
    for(int index = optind; index<argc; index++){
        printf ("Non-option argument \"%s\"\n", argv[index]);
        exit(EXIT_FAILURE);
    }

    //PARSING CONFIG
    struct struct_costanti * costanti=malloc(sizeof(struct struct_costanti));
    costanti->LOG=malloc(sizeof(char)*BUFFERSIZE);
    FILE * config;
    config = fopen("./config.txt", "r");
    CHECK_PTR(config,"Apertura file di configurazione");
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->K)),"Lettura K");
    if((costanti->K)<=0){
        perror("Cassieri totali nulli o negativi");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->K_ini)),"Lettura K_ini");
    if((costanti->K_ini)<=0 || (costanti->K_ini)>(costanti->K)){
        perror("Cassieri iniziali nulli, negativi o superiori alle casse disponibili");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->C)),"Lettura C");
    if((costanti->C)<=0){
        perror("Clienti massimi nulli o negativi");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->E)),"Lettura E");
    if((costanti->E)<=0 || (costanti->E)>(costanti->C)){
        perror("Scaglione di ingresso nullo, negativo o superiore ai clienti massimi");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->T)),"Lettura T");
    if((costanti->T)<=0){
        perror("Tempo di scelta prodotti nullo o negativo");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->TP)),"Lettura TP");
    if((costanti->TP)<=0){
        perror("Tempo per prodotto nullo o negativo");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->P)),"Lettura P");
    if((costanti->P)<=0){
        perror("Prodotti massimi nullo o negativo");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n",  &(costanti->S1)) ,"Lettura S1");
    if((costanti->S1)<=0 || (costanti->S1)>(costanti->K)){
        perror("Soglia 1 nulla, negativa o superiore alle casse disponibili");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->S2)),"Lettura S2");
    if((costanti->S2)<=0 || (costanti->S2)>(costanti->C)){
        perror("Soglia 2 nulla, negativa o superiore ai clienti massimi");
        exit(EXIT_FAILURE);
    }
    CHECK_ERR(fscanf(config, "%*[^=]%*c%d\n", &(costanti->I)) ,"Lettura I");
    if((costanti->I)<=0){
        perror("Intervallo di comunicazione nullo o negativo");
        exit(EXIT_FAILURE);
    }

    if(logflag) strcpy(costanti->LOG,tmplog);
    else{
        CHECK_ERR(fscanf(config, "%*[^=]%*c%s\n", costanti->LOG),"Lettura LOG");
        if((costanti->LOG)==NULL){
            perror("Nome file di log nullo");
            exit(EXIT_FAILURE);
        }
    }

    CHECK_ERR(fclose(config),"Chiusura file di configurazione");

    //CREAZIONE DEL DIRETTORE
    pthread_t * direttore=malloc(sizeof(pthread_t));
    if((pthread_create(direttore, NULL, fun_direttore, costanti))!=0){
        fprintf(stderr, "Creazione thread direttore fallita\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(*direttore, NULL) == -1) {
        fprintf(stderr, "pthread_join direttore failed\n");
    }

    //FREES
    free(tmplog);
    free(costanti->LOG);
    free(costanti);
    free(direttore);
    printf("### SUPERMERCATO CHIUSO ###\n");
    return 0;
}
