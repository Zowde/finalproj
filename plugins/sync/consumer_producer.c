#include "consumer_producer.h"
#include <stdlib.h>
#include <string.h>

const char* consumer_producer_init(consumer_producer_t* queue, int capacity) {
    if (queue == NULL || capacity <= 0) {
        return "Invalid queue or capacity";
    }
    
    /* Allocate array of string pointers */
    queue->items = (char**)malloc(capacity * sizeof(char*));
    if (queue->items == NULL) {
        return "Memory allocation failed for queue items";
    }
    
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    
    /* Initialize monitors */
    if (monitor_init(&queue->not_full_monitor) != 0) {
        free(queue->items);
        return "Failed to initialize not_full_monitor";
    }
    
    if (monitor_init(&queue->not_empty_monitor) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);
        return "Failed to initialize not_empty_monitor";
    }
    
    if (monitor_init(&queue->finished_monitor) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        monitor_destroy(&queue->not_empty_monitor);
        free(queue->items);
        return "Failed to initialize finished_monitor";
    }
    
    /* Queue starts not full */
    monitor_signal(&queue->not_full_monitor);
    
    return NULL;  /* Success */
}

void consumer_producer_destroy(consumer_producer_t* queue) {
    if (queue == NULL) {
        return;
    }
    
    /* Free all remaining items in queue */
    while (queue->count > 0) {
        free(queue->items[queue->head]);
        queue->head = (queue->head + 1) % queue->capacity;
        queue->count--;
    }
    
    /* Free the items array */
    free(queue->items);
    
    /* Destroy monitors */
    monitor_destroy(&queue->not_full_monitor);
    monitor_destroy(&queue->not_empty_monitor);
    monitor_destroy(&queue->finished_monitor);
}

const char* consumer_producer_put(consumer_producer_t* queue, const char* item) {
    if (queue == NULL || item == NULL) {
        return "Invalid queue or item";
    }
    
    /* Make a copy of the string */
    char* item_copy = (char*)malloc(strlen(item) + 1);
    if (item_copy == NULL) {
        return "Memory allocation failed for item";
    }
    strcpy(item_copy, item);
    
    /* Wait for queue to not be full */
    if (monitor_wait(&queue->not_full_monitor) != 0) {
        free(item_copy);
        return "Wait on not_full_monitor failed";
    }
    
    /* Add item to queue */
    queue->items[queue->tail] = item_copy;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    /* If queue is full now, reset the not_full signal */
    if (queue->count >= queue->capacity) {
        monitor_reset(&queue->not_full_monitor);
    }
    
    /* Signal that queue is not empty */
    monitor_signal(&queue->not_empty_monitor);
    
    return NULL;  /* Success */
}

char* consumer_producer_get(consumer_producer_t* queue) {
    if (queue == NULL) {
        return NULL;
    }
    
    /* Wait for queue to not be empty */
    if (monitor_wait(&queue->not_empty_monitor) != 0) {
        return NULL;
    }
    
    /* Get item from queue */
    char* item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    /* If queue is empty now, reset the not_empty signal */
    if (queue->count == 0) {
        monitor_reset(&queue->not_empty_monitor);
    }
    
    /* Signal that queue is not full */
    monitor_signal(&queue->not_full_monitor);
    
    return item;  /* Caller takes ownership */
}

void consumer_producer_signal_finished(consumer_producer_t* queue) {
    if (queue == NULL) {
        return;
    }
    
    monitor_signal(&queue->finished_monitor);
}

int consumer_producer_wait_finished(consumer_producer_t* queue) {
    if (queue == NULL) {
        return -1;
    }
    
    return monitor_wait(&queue->finished_monitor);
}