#ifndef CLIENTE_H
#define CLIENTE_H
#include <stdlib.h>
#include <pthread.h>
#include <linkedlist.h>
#include <cassiere.h>

struct struct_cliente{
    int id;                             //IDENTIFICATIVO INCREMENTALE E UNIVOCO DEL CLIENTE
    struct struct_costanti * costanti;  //STRUTTURA CONTENENTE LE COSTANTI CONDIVISE DAGLI ATTORI DEL SISTEMA
    pthread_t thread;                   //THREAD DEL CLIENTE
    pthread_mutex_t * cliente_mutex;    //MUTEX CHE PROTEGGE LE MODIFICHE CONCORRENZIALI DI CAMPI DEL CLIENTE
    int prodotti;                       //Quantità di prodottiQUANTITÀ DI PRODOTTI ACQUISTATI DAL CLIENTE
    int delay;                          //TEMPO CHE IMPIEGA IL CLIENTE A "SCEGLIERE I PRODOTTI" PRIMA DI METTERSI IN CODA
    struct struct_cassiere ** casse;    //ARRAY DI CASSE FRA LE QUALI IL CLIENTE PUÒ DECIDERE DI METTERSI IN CODA
    pthread_cond_t * being_serv;        //VARIABILE DI CONDIZIONE CHE RAPPRESENTA L'AVVENUTO SERVIZIO DA PARTE DEL CASSIERE
    linkedlist * noprod;                //LISTA NELLA QUALE IL CLIENTE SI DOVRÀ INSERIRE SE DECIDE DI NON EFFETTUARE ACQUISTI
    pthread_cond_t * permesso_uscita;   //VARIABILE DI CONDIZIONE SULLA QUALE IL CLIENTE, SE DECIDE DI NON ACQUISTARE PRODOTTI, ATTENDE IL PERMESSO DEL DIRETTORE PER USCIRE
    int servito;                        //FLAG CHE INDICA L'AVVENUTO SERVIZIO DA PARTE DEL CASSIERE
    int cambio_coda;                    //FLAG CHE INDICA SE IL CLIENTE È STATO OBBLIGATO A CAMBIARE CODA IN SEGUITO ALLA CHIUSURA DI UNA CASSA NELLA CUI CODA SI TROVAVA
    int quit;                           //FLAG CHE INDICA AL CLIENTE DI DOVER USCIRE FORZATAMENTE DAL SUPERMERCATO
    int tempo_ingresso;                 //MISURAZIONE DEI MILLISECONDI PER I QUALI UNA CASSA È STATA IN ATTIVITÀ SULLA QUALE IL CLIENTE SI BASA PER CAPIRE QUANTO TEMPO HA SPESO IN CODA
    int tempotot;                       //TEMPO CHE IL CLIENTE PASSA ALL'INTERNO DEL SUPERMERCATO (COMPRENSIVO DI SCELTA PRODOTTI E ATTESA IN TUTTE LE CODE CHE HA VISITATO)
    int code_visitate;                  //CODE NELLE QUALI SI È INSERITO IL CLIENTE
    int staff;                          //FLAG CLIENTE FITTIZIO
    int tokill;                         //INDICE CASSA CHE IL CLIENTE FITTIZIO DEVE CHIUDERE
};

void cliente_initilize(struct struct_cliente ** self);
void clientefittizio_initilize(struct struct_cliente ** self);
int get_delay_cliente(unsigned int * seed, int bound);
int get_tobuy (unsigned int *seed, int bound);
void scelta_coda(struct struct_cliente ** self);
void avvisa_cassiere(struct struct_cliente ** self);
void scelta_prodotti (int attesa);
void cliente_cleanup (void* arg);
void clientefittizio_cleanup (void* arg);
#endif
