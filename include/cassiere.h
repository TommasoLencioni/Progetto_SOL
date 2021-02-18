#ifndef CASSIERE_H
#define CASSIERE_H
#include <pthread.h>
#include <linkedlist.h>
#include <time.h>
struct struct_cassiere{
      int id;                               //IDENTIFICATIVO INCREMENTAE E UNIVOCO DELLA CASSA
      int status;                           //0 CASSA CHIUSA, 1 CASSA APERTA
      struct struct_costanti * costanti;    //STRUTTURA CONTENENTE LE COSTANTI CONDIVISE DAGLI ATTORI DEL SISTEMA
      pthread_t thread;                     //THREAD DEL CASSIERE
      int delay;                            //TEMPO FISSO CHE IMPIEGA A SERVIRE UN CLIENTE
      pthread_mutex_t * coda_mutex;         //MUTEX PER L'UTILIZZO DELLA CODA DELLA CASSA IN MUTUA ESCLUSIONE
      pthread_mutex_t * self;               //MUTEX PER PROTEGGERE LE MODIFICHE ALLO STATUS DELLA CASSA
      pthread_cond_t * empty;               //VARIABILE DI CONDIZIONE CHE RAPPRESENTA LA CODA IN CASSA VUOTA
      linkedlist * in_coda;                 //LINKED LIST FIFO DI CLIENTI IN CODA
      int serviti;                          //NUMERO DI CLIENTI SERVITI
      int prod_elaborati;                   //NUMERO DI PRODOTTI ELABORATI
      int old_tempo_apertura;               //TEMPO DI APERTURA AL CLIENTE PRECEDENTE
      int tempo_apertura;                   //TEMPO DI APERTURA ATTUALE
      int chiusure;                         //NUMERO DI CHIUSURE
      int ordine_chiusura;                  //FLAG CHE INDICA AL CASSIERE CHE IL SUPERMERCATO STA CHIUDENDO
      int pfd[2];                           //PIPE PER LA COMUNICAZIONE CASSIERE-DIRETTORE
};

void cassiere_initialize (struct struct_cassiere ** cassiere);
int get_delay_cassiere(unsigned int * seed);
void serve(struct struct_cassiere ** self, struct node * primo);
void sposta (struct struct_cassiere ** self, struct node * primo);
void fa_uscire(struct struct_cassiere ** self, struct node * primo);
void cassiere_cleanup (void* arg);
#endif
