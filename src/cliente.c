#include <stdio.h>
#include <time.h>
#include <cliente.h>
#include <pthread.h>
#include <linkedlist.h>
#include <cassiere.h>
#include <supermercato.h>

void cliente_initilize(struct struct_cliente ** self){
    //MUTEX DEL CLIENTE
    pthread_mutex_t * mtx=malloc(sizeof(pthread_mutex_t));
    CHECK_ERR(pthread_mutex_init(mtx,NULL),"Creazione mutex cliente");
    (*self)->cliente_mutex=mtx;
    CHECK_ERR(pthread_mutex_lock((*self)->cliente_mutex),"Acquisizione lock");
    //ASSEGNO DELAY
    unsigned int seed = time(NULL) * (*self)->id;
    int delay=get_delay_cliente(&seed, ((*self)->costanti)->T);
    (*self)->delay=delay;
    //ASSEGNO PRODOTTI
    int prodotti=get_tobuy(&seed, ((*self)->costanti)->P);
    (*self)->prodotti=prodotti;
    //INIZIALIZZAZIONE VARIABILE DI CONDIZIONE SERVIZIO
    pthread_cond_t * being_serv = malloc(sizeof(pthread_cond_t));
    CHECK_ERR(pthread_cond_init(being_serv, NULL),"Creazione variabile di condizione servizio");
    (*self)->being_serv=being_serv;
    //INIZIALIZZAZIONE VARIABILE DI CONDIZIONE PERMESSO USCITA
    pthread_cond_t * permesso = malloc(sizeof(pthread_cond_t));
    CHECK_ERR(pthread_cond_init(permesso, NULL),"Creazione variabile di condizione permesso");
    (*self)->permesso_uscita=permesso;
    //FLAGS E CONTATORI
    (*self)->servito=0;
    (*self)->cambio_coda=1;
    (*self)->tempo_ingresso=0;
    (*self)->tempotot=(*self)->delay;
    (*self)->code_visitate=0;
    (*self)->staff=0;
    (*self)->quit=0;
    CHECK_ERR(pthread_mutex_unlock((*self)->cliente_mutex),"Rilascio lock");
    return;
}

void clientefittizio_initilize(struct struct_cliente ** self){
    pthread_mutex_t * mtx=malloc(sizeof(pthread_mutex_t));
    //MUTEX DEL CLIENTE
    CHECK_ERR(pthread_mutex_init(mtx,NULL),"Creazione mutex cliente");
    (*self)->cliente_mutex=mtx;
    CHECK_ERR(pthread_mutex_lock((*self)->cliente_mutex),"Acquisizione lock");
    //INIZIALIZZAZIONE VARIABILE DI CONDIZIONE SERVIZIO
    pthread_cond_t * being_serv = malloc(sizeof(pthread_cond_t));
    CHECK_ERR(pthread_cond_init(being_serv, NULL),"Creazione variabile di condizione servizio");
    //FLAGS E CONTATORI
    (*self)->being_serv=being_serv;
    (*self)->servito=0;
    (*self)->staff=1;
    (*self)->servito=0;
    (*self)->quit=0;
    CHECK_ERR(pthread_mutex_unlock((*self)->cliente_mutex),"Rilascio lock");
    return;
}

void scelta_coda(struct struct_cliente ** self){
    unsigned int seed;
    int chosen;
    struct struct_cassiere ** casse=(*self)->casse;
    CHECK_ERR(pthread_mutex_lock((*self)->cliente_mutex),"Acquisizione lock me stesso");
    while(!((*self)->servito)){
        seed=time(NULL)*(*self)->id;
        chosen=rand_r(&seed)%((*self)->costanti->K);
        if((*self)->cambio_coda){ //CONSUMO IL CAMBIO CODA
            (*self)->code_visitate++;
        }
        (*self)->cambio_coda=0;
        while(1){
            chosen=chosen%((*self)->costanti->K);
            CHECK_ERR(pthread_mutex_lock((*casse[chosen]).self),"Acquisizione lock cassa");
            if((*casse[chosen]).status==1){
                CHECK_ERR(pthread_mutex_unlock((*casse[chosen]).self),"Rilascio lock cassa");
                insert_tail(self,&(casse[chosen])->in_coda,(*casse[chosen]).coda_mutex,(*casse[chosen]).empty);
                (*self)->tempo_ingresso=(*casse[chosen]).tempo_apertura;
                break;
            }
            else{
                CHECK_ERR(pthread_mutex_unlock((*casse[chosen]).self),"Rilascio lock cassa");
                chosen++;
            }
        }
        while(!(*self)->servito && !((*self)->cambio_coda)){ // && !(*self)->quit
            CHECK_ERR(pthread_cond_wait((*self)->being_serv,(*self)->cliente_mutex),"Wait cliente");
        }
        CHECK_ERR(pthread_mutex_lock((*casse[chosen]).self),"Acquisizione lock cassa");
        if(((*self)->cambio_coda)) (*self)->tempotot+=((*casse[chosen]).tempo_apertura)-((*self)->tempo_ingresso);
        CHECK_ERR(pthread_mutex_unlock((*casse[chosen]).self),"Rilascio lock cassa");
    }
    CHECK_ERR(pthread_mutex_unlock((*self)->cliente_mutex),"Rilascio lock");
    return;
}

void avvisa_cassiere(struct struct_cliente ** self){
    CHECK_ERR(pthread_mutex_lock((*self)->cliente_mutex),"Acquisizione lock me stesso");
    struct struct_cassiere ** casse=(*self)->casse;
    while(!((*self)->servito)){
        insert_tail(self,&(*casse[(*self)->tokill]).in_coda,(*casse[(*self)->tokill]).coda_mutex,(*casse[(*self)->tokill]).empty);
        while(!(*self)->servito){
            CHECK_ERR(pthread_cond_wait((*self)->being_serv,(*self)->cliente_mutex),"Wait cliente");
        }
    }
    CHECK_ERR(pthread_mutex_unlock((*self)->cliente_mutex),"Rilascio lock");
    return;
}

int get_delay_cliente(unsigned int * seed, int bound){
    int attesa=rand_r(seed)%(bound-10);
    attesa+=10;
    return attesa;
}

int get_tobuy (unsigned int *seed, int bound){
    int prodotti=rand_r(seed)%(bound+1);
    return prodotti;
}

void scelta_prodotti (int attesa){
    struct timespec tempo;
    tempo.tv_sec=0;
    tempo.tv_nsec = attesa*1000000;
    while (nanosleep(&tempo, &tempo));
    return;
}

void cliente_cleanup (void* arg)
{
    struct struct_cliente * argomenti=arg;
    free(argomenti->cliente_mutex);
    free(argomenti->being_serv);
    free(argomenti->permesso_uscita);
    free(argomenti);
}

void clientefittizio_cleanup (void* arg)
{
    struct struct_cliente * argomenti=arg;
    free(argomenti->cliente_mutex);
    free(argomenti->being_serv);
    free(argomenti);
}
