#include <stdlib.h>
#include <pthread.h>
#include <linkedlist.h>
#ifndef SUPERMERCATO_H
#define SUPERMERCATO_H

    #define BUFFERSIZE 1024
    #define CHECK_PTR(ptr,stringa) \
       if(ptr==NULL){ 	\
           perror(stringa);	\
           exit(EXIT_FAILURE);	\
       }

    #define CHECK_ERR(v,m) \
      if(v==-1){  \
          perror(m);  \
          exit(EXIT_FAILURE); \
      }

    struct struct_costanti{
        int K;          //NUMERO DI CASSIERI TOTALI, COSTANTE DA FILE DI CONFIGURAZIONE
        int K_ini;      //NUMERO DI CASSIERI DA APRIRE INIZIALMENTE
        int K_open;     //NUMERO DI CASSE ATTUALMENTE APERTE (utilizzo protetto da mtxcasse)
        int C;          //NUMERO DI CLIENTI MASSIMI ALL'INTERNO DEL SUPERMERCATO, COSTANTE DA FILE DI CONFIGURAZIONE
        int C_id;       //IDENTIFICATIVO UNIVOCO E INCREMENTALE DA ASSEGNARE AD OGNI CLENTE CHE ENTRA
        int C_in;       //NUMERO DI CLIENTI ATTUALMENTE ALL'INTERNO DEL SUPERMERCATO (utilizzo protetto da mtxclienti)
        int E;          //SCAGLIONE DI INGRESSO, COSTANTE DA FILE DI CONFIGURAZIONE
        int T;          //TEMPO MASSIMO SCEKTA PRODOTTI, COSTANTE DA FILE DI CONFIGURAZIONE
        int TP;         //TEMPO IMPIEGATO DA UN QUALSIASI CASSIERE A ELABORARE UN PRODOTTO
        int P;          //PRODOTTI MASSIMI CHE UN CLIENTE PUO' ACQUISTARE
        int S1;         //SOGLIA 1
        int S2;         //SOGLIA 2
        int I;          //INTERVALLO COMUNCIAZIONE CASSE-DIRETTORE
        char * LOG;     //NOME DEL FILE DI LOG
    };

    struct struct_ingressi{
        struct struct_costanti * costanti;
        struct struct_cassiere ** casse;
        linkedlist * noprod;
    };

    struct struct_noprod{
        linkedlist * lista;
        pthread_cond_t * empty;
    };

    extern pthread_mutex_t mtxclienti; //MUTEX PER L'UTILIZZO DEL NUMERO DI CLIENTI E IL LORO ID
    void attesa (int attesa);

#endif
