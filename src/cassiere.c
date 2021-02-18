#include <stdio.h>
#include <time.h>
#include <supermercato.h>
#include <cassiere.h>
#include <cliente.h>
#include <pthread.h>
#include <linkedlist.h>

void cassiere_initialize (struct struct_cassiere ** self){
    //CREAZIONE MUTEX CODA
    pthread_mutex_t * mtx=malloc(sizeof(pthread_mutex_t));
    CHECK_ERR(pthread_mutex_init(mtx,NULL),"Creazione mutex coda");
    (*self)->coda_mutex=mtx;
    //CREAZIONE MUTEX CASSA
    pthread_mutex_t * mtxself=malloc(sizeof(pthread_mutex_t));
    CHECK_ERR(pthread_mutex_init(mtxself,NULL),"Creazione mutex coda");
    (*self)->self=mtxself;
    //VARIABILE DI CONDIZIONE CODA VUOTA
    pthread_cond_t * empty = malloc(sizeof(pthread_cond_t));
    CHECK_ERR(pthread_cond_init(empty, NULL),"Creazione variabile di condizione");
    (*self)->empty=empty;
    //GENERAZIONE E ASSEGNAZIONE DELAY
    unsigned int seed = time(NULL) * (*self)->id;
    int delay=get_delay_cassiere(&seed);
    (*self)->delay=delay;
    //CREAZIONE E INIZIALIZZAZIONE LINKED LIST PER LA CODA
    (*self)->in_coda=malloc(sizeof(linkedlist));
    CHECK_PTR((*self)->in_coda,"Allocazione in coda");
    linkedlist_initializer(&(*self)->in_coda);
    //FLAGS E CONTATORI
    (*self)->status=0;
    (*self)->serviti=0;
    (*self)->prod_elaborati=0;
    (*self)->tempo_apertura=0;
    (*self)->chiusure=0;
    (*self)->ordine_chiusura=0;
    return;
}

void serve(struct struct_cassiere ** self, struct node * primo){
    struct struct_cliente ** cliente = (*primo).data;
    CHECK_ERR(pthread_mutex_lock((*cliente)->cliente_mutex),"Acquisizione lock");
    if((*cliente)->staff) (*self)->ordine_chiusura=1;
    else{
        int tempo_di_servizio=(*self)->delay+((*cliente)->prodotti*((*self)->costanti->TP));
        (*cliente)->tempotot+=((*self)->tempo_apertura)-((*cliente)->tempo_ingresso);
        attesa(tempo_di_servizio);
        (*self)->serviti++;
        (*self)->prod_elaborati+=(*cliente)->prodotti;
        (*self)->tempo_apertura+=tempo_di_servizio;
    }
    (*cliente)->servito=1;
    CHECK_ERR(pthread_cond_signal((*cliente)->being_serv),"Signal cliente servito");
    CHECK_ERR(pthread_mutex_unlock((*cliente)->cliente_mutex),"Rilascio lock");
    return;
}

void fa_uscire (struct struct_cassiere ** self, struct node * primo){
    struct struct_cliente ** cliente = (*primo).data;
    CHECK_ERR(pthread_mutex_lock((*cliente)->cliente_mutex),"Acquisizione lock");
    if((*cliente)->staff) (*self)->ordine_chiusura=1;
    (*cliente)->quit=1;
    (*cliente)->servito=1;
    CHECK_ERR(pthread_cond_signal((*cliente)->being_serv),"Signal cliente servito");
    CHECK_ERR(pthread_mutex_unlock((*cliente)->cliente_mutex),"Rilascio lock");
    if(!(*self)->ordine_chiusura){
        CHECK_ERR(pthread_mutex_lock(&mtxclienti),"Acquisizione lock clienti");
        ((*self)->costanti)->C_in--;
        CHECK_ERR(pthread_mutex_unlock(&mtxclienti),"Rilascio lock clienti");
    }
    return;
}

void sposta (struct struct_cassiere ** self, struct node * primo){
    struct struct_cliente ** cliente = (*primo).data;
    CHECK_ERR(pthread_mutex_lock((*cliente)->cliente_mutex),"Acquisizione lock");
    (*cliente)->cambio_coda=1;
    CHECK_ERR(pthread_cond_signal((*cliente)->being_serv),"Signal cliente servito");
    CHECK_ERR(pthread_mutex_unlock((*cliente)->cliente_mutex),"Rilascio lock");
    return;
}

int get_delay_cassiere(unsigned int * seed){
    int attesa=rand_r(seed)%60;
    attesa+=20;
    return attesa;
}

void cassiere_cleanup (void* arg){
    struct struct_cassiere * argomenti=arg;
    free(argomenti->coda_mutex);
    free(argomenti->self);
    free(argomenti->empty);
    free(argomenti->in_coda);
    free(argomenti);
}
