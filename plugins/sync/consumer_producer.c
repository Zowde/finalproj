/* */
#include "consumer_producer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Need _GNU_SOURCE for strdup */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>


const char* consumer_producer_init(consumer_producer_t* queue, int capacity) { /* */
    if (capacity <= 0) {
        return "Queue capacity must be positive.";
    }
    queue->items = malloc(sizeof(char*) * capacity); /* */
    if (!queue->items) {
        return "Failed to allocate memory for queue items.";
    }
    queue->capacity = capacity; /* */
    queue->count = 0; /* */
    queue->head = 0; /* */
    queue->tail = 0; /* */
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->items);
        return "Failed to initialize queue mutex.";
    }
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->items);
        return "Failed to initialize not_full condition variable.";
    }
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->not_full);
        free(queue->items);
        return "Failed to initialize not_empty condition variable.";
    }
    if (monitor_init(&queue->finished_monitor) != 0) { /* */
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->not_full);
        pthread_cond_destroy(&queue->not_empty);
        free(queue->items);
        return "Failed to initialize finished monitor.";
    }
    
    return NULL; /* */
}

void consumer_producer_destroy(consumer_producer_t* queue) { /* */
    /* Free any remaining items in the queue */
    for (int i = 0; i < queue->count; i++) {
        free(queue->items[(queue->tail + i) % queue->capacity]);
    }
    
    free(queue->items); /* */
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
    monitor_destroy(&queue->finished_monitor); /* */
}

const char* consumer_producer_put(consumer_producer_t* queue, const char* item) { /* */
    pthread_mutex_lock(&queue->mutex); /* */
    
    /* Wait until there is space in the queue */
    while (queue->count == queue->capacity) { /* */
        pthread_cond_wait(&queue->not_full, &queue->mutex); /* */
    }
    
    /* We must copy the string, as the queue takes ownership */
    char* new_item = strdup(item);
    if (!new_item) {
        pthread_mutex_unlock(&queue->mutex);
        return "Failed to duplicate string for queue.";
    }
    
    queue->items[queue->head] = new_item; /* */
    queue->head = (queue->head + 1) % queue->capacity; /* */
    queue->count++; /* */
    
    /* Signal that the queue is no longer empty */
    pthread_cond_signal(&queue->not_empty);
    
    pthread_mutex_unlock(&queue->mutex); /* */
    return NULL; /* */
}

char* consumer_producer_get(consumer_producer_t* queue) { /* */
    pthread_mutex_lock(&queue->mutex); /* */
    
    /* Wait until there is an item in the queue */
    while (queue->count == 0) { /* */
        pthread_cond_wait(&queue->not_empty, &queue->mutex); /* */
    }
    
    char* item = queue->items[queue->tail]; /* */
    queue->items[queue->tail] = NULL; /* Avoid dangling pointer */
    queue->tail = (queue->tail + 1) % queue->capacity; /* */
    queue->count--; /* */
    
    /* Signal that the queue is no longer full */
    pthread_cond_signal(&queue->not_full);
    
    pthread_mutex_unlock(&queue->mutex); /* */
    return item; /* */
}

void consumer_producer_signal_finished(consumer_producer_t* queue) { /* */
    monitor_signal(&queue->finished_monitor); /* */
}

int consumer_producer_wait_finished(consumer_producer_t* queue) { /* */
    monitor_wait(&queue->finished_monitor); /* */
    return 0; /* */
}