/* * [source: 437] Unit test application for consumer_producer.c
 * This file is required by the new build.sh
 */
#include "consumer_producer.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define QUEUE_CAPACITY 5
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 2
#define ITEMS_PER_PRODUCER 20
#define TOTAL_ITEMS (NUM_PRODUCERS * ITEMS_PER_PRODUCER)

consumer_producer_t test_queue;
int consumed_item_count = 0;
pthread_mutex_t count_mutex;

/* [source: 408] Producer thread function */
void* producer_func(void* arg) {
    int producer_id = *(int*)arg;
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        char item_str[50];
        snprintf(item_str, sizeof(item_str), "item-%d-%d", producer_id, i);
        
        // [source: 414]
        const char* err = consumer_producer_put(&test_queue, item_str);
        assert(err == NULL);
    }
    return NULL;
}

/* [source: 416] Consumer thread function */
void* consumer_func(void* arg) {
    while (1) {
        // [source: 421]
        char* item = consumer_producer_get(&test_queue);
        
        if (strcmp(item, "<END>") == 0) {
            free(item);
            // Re-broadcast the <END> signal for other consumers
            consumer_producer_put(&test_queue, "<END>");
            break;
        }
        
        // We got an item
        // printf("Consumed: %s\n", item);
        free(item);
        
        pthread_mutex_lock(&count_mutex);
        consumed_item_count++;
        pthread_mutex_unlock(&count_mutex);
    }
    return NULL;
}

/* [source: 366-372] Test: Multiple producers, multiple consumers, forced blocking */
void test_multi_producer_consumer() {
    printf("[TEST] Running: Multi-Producer/Consumer Stress Test\n");
    
    // [source: 401]
    const char* err = consumer_producer_init(&test_queue, QUEUE_CAPACITY);
    assert(err == NULL);
    pthread_mutex_init(&count_mutex, NULL);
    
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int producer_ids[NUM_PRODUCERS];

    printf("  [TEST] Starting %d consumer threads...\n", NUM_CONSUMERS);
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_create(&consumers[i], NULL, consumer_func, NULL);
    }
    
    printf("  [TEST] Starting %d producer threads (each producing %d items)...\n", NUM_PRODUCERS, ITEMS_PER_PRODUCER);
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i;
        pthread_create(&producers[i], NULL, producer_func, &producer_ids[i]);
    }
    
    // Wait for all producers to finish
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    printf("  [TEST] All producers finished.\n");
    
    // Send the <END> signal to stop consumers
    consumer_producer_put(&test_queue, "<END>");
    
    // Wait for all consumers to finish
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }
    printf("  [TEST] All consumers finished.\n");

    // [source: 406]
    consumer_producer_destroy(&test_queue);
    pthread_mutex_destroy(&count_mutex);
    
    printf("  [TEST] Total items produced: %d\n", TOTAL_ITEMS);
    printf("  [TEST] Total items consumed: %d\n", consumed_item_count);
    
    assert(consumed_item_count == TOTAL_ITEMS);
    
    printf("[TEST] PASS\n\n");
}


int main() {
    printf("--- Running Consumer-Producer Unit Tests ---\n\n");
    
    test_multi_producer_consumer();
    
    printf("--- All Consumer-Producer Tests Passed ---\n");
    return 0;
}
