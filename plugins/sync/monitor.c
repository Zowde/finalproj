#include "monitor.h"
#include <stdlib.h>
#include <errno.h>

int monitor_init(monitor_t* monitor) {
    if (monitor == NULL) {
        return -1;
    }
    
    /* Initialize mutex */
    if (pthread_mutex_init(&monitor->mutex, NULL) != 0) {
        return -1;
    }
    
    /* Initialize condition variable */
    if (pthread_cond_init(&monitor->condition, NULL) != 0) {
        pthread_mutex_destroy(&monitor->mutex);
        return -1;
    }
    
    /* Initialize signaled flag to 0 (not signaled) */
    monitor->signaled = 0;
    
    return 0;
}

void monitor_destroy(monitor_t* monitor) {
    if (monitor == NULL) {
        return;
    }
    
    pthread_cond_destroy(&monitor->condition);
    pthread_mutex_destroy(&monitor->mutex);
}

void monitor_signal(monitor_t* monitor) {
    if (monitor == NULL) {
        return;
    }
    
    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 1;
    pthread_cond_signal(&monitor->condition);
    pthread_mutex_unlock(&monitor->mutex);
}

void monitor_reset(monitor_t* monitor) {
    if (monitor == NULL) {
        return;
    }
    
    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 0;
    pthread_mutex_unlock(&monitor->mutex);
}

int monitor_wait(monitor_t* monitor) {
    if (monitor == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&monitor->mutex);
    
    /* Wait until signaled flag is set */
    while (monitor->signaled == 0) {
        if (pthread_cond_wait(&monitor->condition, &monitor->mutex) != 0) {
            pthread_mutex_unlock(&monitor->mutex);
            return -1;
        }
    }
    
    pthread_mutex_unlock(&monitor->mutex);
    return 0;
}