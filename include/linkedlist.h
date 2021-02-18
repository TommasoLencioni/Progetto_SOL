#ifndef LINKEDLIST_H
#define LINKEDLIST_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//STRUTTURA DELLA LINKEDLIST
typedef struct _linkedlist_ {
    struct node * head;
    struct node * tail;
    int count;
} linkedlist;

//STRUTTURA DEL NODO
struct node {
  void * data;
  struct node * next;
};

void linkedlist_initializer (linkedlist ** list);
int insert_head(void * elem, linkedlist ** list, pthread_mutex_t * mutex, pthread_cond_t * empty);
int insert_tail(void * elem, linkedlist ** list, pthread_mutex_t * mutex, pthread_cond_t * empty);
void * pop_head (linkedlist ** list, pthread_mutex_t * mutex, pthread_cond_t * empty);
int linkedlist_count(linkedlist * list, pthread_mutex_t * mutex);

#endif
