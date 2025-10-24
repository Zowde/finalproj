/* */
#ifndef CONSUMER_PRODUCER_H
#define CONSUMER_PRODUCER_H

#include "monitor.h"
#include <pthread.h>

/**
* Consumer-Producer queue structure
*/
typedef struct
{
 	char** items; 			/* */
 	int capacity; 			/* */
 	int count; 				/* */
 	int head; 				/* */
 	int tail; 				/* */

 	monitor_t not_full_monitor;
    monitor_t not_empty_monitor;
 	monitor_t finished_monitor; /* */
 	
} consumer_producer_t; /* */

/**
* Initialize a consumer-producer queue
* @param queue Pointer to queue structure
* @param capacity Maximum number of items
* @return NULL on success, error message on failure
*/
const char* consumer_producer_init(consumer_producer_t* queue, int capacity); /* */

/**
* Destroy a consumer-producer queue and free its resources
* @param queue Pointer to queue structure
*/
void consumer_producer_destroy(consumer_producer_t* queue); /* */

/**
* Add an item to the queue (producer).
* Blocks if queue is full. 
* @param queue Pointer to queue structure
* @param item String to add (queue takes ownership)
* @return NULL on success, error message on failure
*/
const char* consumer_producer_put(consumer_producer_t* queue, const char* item); /* */

/**
* Remove an item from the queue (consumer) and returns it.
* Blocks if queue is empty. 
* @param queue Pointer to queue structure
* @return String item or NULL if queue is empty
*/
char* consumer_producer_get(consumer_producer_t* queue); /* */

/**
* Signal that processing is finished
* @param queue Pointer to queue structure
*/
void consumer_producer_signal_finished(consumer_producer_t* queue); /* */

/**
* Wait for processing to be finished
* @param queue Pointer to queue structure
* @return 0 on success
*/
int consumer_producer_wait_finished(consumer_producer_t* queue); /* */


#endif // CONSUMER_PRODUCER_H

