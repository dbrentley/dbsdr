//
// Created by dbrent on 2/20/21.
//

#ifndef DBSDR_QUEUE_H
#define DBSDR_QUEUE_H

#include <pthread.h>


typedef struct queue_entry {
    struct queue_entry *next;
    struct queue_entry *prev;
    void *val;
} queue_entry_t;


typedef struct {
    queue_entry_t *first;
    queue_entry_t *last;
    pthread_mutex_t lock;
    size_t size;
} queue_t;


void queue_init(queue_t *q);

void queue_destroy(queue_t *q);

void queue_append(queue_t *q, void *val);

void *queue_pop(queue_t *q);

size_t queue_size(queue_t *q);

#endif //DBSDR_QUEUE_H
