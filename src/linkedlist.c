#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <linkedlist.h>

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

void linkedlist_initializer (linkedlist ** list){
    (*list)->head=NULL;
    (*list)->tail=NULL;
    (*list)->count=0;
}

int insert_head(void * elem, linkedlist ** list, pthread_mutex_t * mutex, pthread_cond_t * empty){
    if(mutex!=NULL){
        pthread_mutex_lock(mutex);
    }
    struct node *new;
    new = malloc(sizeof(struct node));
    CHECK_PTR(new,"Malloc new");
    new->data = elem;
    new->next= (*list)->head;
    if((*list)->head==NULL) (*list)->tail=new;
    (*list)->head=new;
    (*list)->count++;
    if(empty!=NULL) pthread_cond_signal(empty);
    if(mutex!=NULL) pthread_mutex_unlock(mutex);
    return 0;
}

int insert_tail(void * elem, linkedlist ** list, pthread_mutex_t * mutex, pthread_cond_t * empty){
    if(mutex!=NULL){
        CHECK_ERR(pthread_mutex_lock(mutex),"Lock mutex insert tail");
    }
    struct node *new=malloc(sizeof(struct node));
    CHECK_PTR(new,"Malloc new");
    new->data =elem;
    new->next= NULL;
    if((*list)->tail!=NULL) (*list)->tail->next=new;
    else (*list)->head = new;
    (*list)->tail = new;
    (*list)->count++;
    if(empty!=NULL) pthread_cond_signal(empty);
    if(mutex!=NULL) CHECK_ERR(pthread_mutex_unlock(mutex),"Unlock mutex insert tail");
    return 0;
}

void * pop_head (linkedlist ** list, pthread_mutex_t * mutex, pthread_cond_t * empty){
    if(mutex!=NULL){
        pthread_mutex_lock(mutex);
        while(empty && (((*list)->head)==NULL)){
            pthread_cond_wait(empty,mutex);
        }
    }
    if(!(*list)->head && !(mutex && empty)) return NULL;
    void * elem=(*list)->head;
    (*list)->head=(*list)->head->next;
    if((*list)->head==NULL) (*list)->tail=NULL;
    (*list)->count--;
    if(mutex!=NULL) pthread_mutex_unlock(mutex);
    return elem;
}

int linkedlist_count (linkedlist * list, pthread_mutex_t * mutex){
    int c=0;
    if(mutex!=NULL) CHECK_ERR(pthread_mutex_lock(mutex),"Acquisizione mutex");
    c=list->count;
    if(mutex!=NULL) CHECK_ERR(pthread_mutex_unlock(mutex),"Rilascio mutex");
    return c;
}
