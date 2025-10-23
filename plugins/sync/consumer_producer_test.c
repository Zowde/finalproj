#include "consumer_producer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static int test_count = 0;
static int passed = 0;

void print_test(const char* name, int result) {
    test_count++;
    if (result) {
        printf("[PASS] Test %d: %s\n", test_count, name);
        passed++;
    } else {
        printf("[FAIL] Test %d: %s\n", test_count, name);
    }
}

/* Test 1: Basic init and destroy */
void test_init_destroy(void) {
    consumer_producer_t queue;
    const char* err = consumer_producer_init(&queue, 10);
    int success = (err == NULL);
    consumer_producer_destroy(&queue);
    print_test("init_destroy", success);
}

/* Test 2: Put and get single item */
void test_put_get_single(void) {
    consumer_producer_t queue;
    consumer_producer_init(&queue, 10);
    
    const char* err = consumer_producer_put(&queue, "hello");
    char* item = consumer_producer_get(&queue);
    
    int success = (err == NULL && item != NULL && strcmp(item, "hello") == 0);
    free(item);
    consumer_producer_destroy(&queue);
    print_test("put_get_single", success);
}

/* Test 3: Queue capacity and circular buffer */
void test_circular_buffer(void) {
    consumer_producer_t queue;
    consumer_producer_init(&queue, 3);
    
    /* Fill queue */
    consumer_producer_put(&queue, "item1");
    consumer_producer_put(&queue, "item2");
    consumer_producer_put(&queue, "item3");
    
    /* Retrieve and refill to test circular */
    char* item1 = consumer_producer_get(&queue);
    char* item2 = consumer_producer_get(&queue);
    
    consumer_producer_put(&queue, "item4");
    consumer_producer_put(&queue, "item5");
    
    char* item3 = consumer_producer_get(&queue);
    char* item4 = consumer_producer_get(&queue);
    char* item5 = consumer_producer_get(&queue);
    
    int success = (strcmp(item1, "item1") == 0 &&
                   strcmp(item2, "item2") == 0 &&
                   strcmp(item3, "item3") == 0 &&
                   strcmp(item4, "item4") == 0 &&
                   strcmp(item5, "item5") == 0);
    
    free(item1);
    free(item2);
    free(item3);
    free(item4);
    free(item5);
    consumer_producer_destroy(&queue);
    print_test("circular_buffer", success);
}

/* Shared state for blocking tests */
static consumer_producer_t test_queue;
static char* producer_result = NULL;

void* producer_thread(void* arg) {
    (void)arg;  /* Suppress unused parameter warning */
    /* This should block because queue capacity is 1 and consumer hasn't started */
    usleep(500000);  /* Wait 500ms */
    const char* err = consumer_producer_put(&test_queue, "data");
    producer_result = (err == NULL) ? "success" : (char*)err;
    return NULL;
}

void* consumer_thread(void* arg) {
    (void)arg;  /* Suppress unused parameter warning */
    usleep(200000);  /* Wait 200ms, then consume */
    char* item = consumer_producer_get(&test_queue);
    free(item);
    return NULL;
}

/* Test 4: Producer blocks when full */
void test_producer_blocks(void) {
    consumer_producer_init(&test_queue, 1);
    producer_result = NULL;
    
    /* Pre-fill queue */
    consumer_producer_put(&test_queue, "blocking_item");
    
    /* Start producer thread that will block trying to put */
    pthread_t pid;
    pthread_create(&pid, NULL, producer_thread, NULL);
    
    /* Wait a bit, then consume to unblock producer */
    usleep(300000);
    char* item = consumer_producer_get(&test_queue);
    free(item);
    
    pthread_join(pid, NULL);
    consumer_producer_destroy(&test_queue);
    
    int success = (producer_result != NULL && strcmp(producer_result, "success") == 0);
    print_test("producer_blocks_when_full", success);
}

/* Test 5: Multiple items in sequence */
void test_multiple_items(void) {
    consumer_producer_t queue;
    consumer_producer_init(&queue, 10);
    
    const char* items[] = {"first", "second", "third", "fourth"};
    int count = 4;
    
    /* Put all items */
    for (int i = 0; i < count; i++) {
        consumer_producer_put(&queue, items[i]);
    }
    
    /* Get all items and verify order (FIFO) */
    int success = 1;
    for (int i = 0; i < count; i++) {
        char* item = consumer_producer_get(&queue);
        if (item == NULL || strcmp(item, items[i]) != 0) {
            success = 0;
        }
        free(item);
    }
    
    consumer_producer_destroy(&queue);
    print_test("multiple_items_fifo", success);
}

/* Test 6: Signal finished */
void test_signal_finished(void) {
    consumer_producer_t queue;
    consumer_producer_init(&queue, 10);
    
    consumer_producer_signal_finished(&queue);
    int result = consumer_producer_wait_finished(&queue);
    
    consumer_producer_destroy(&queue);
    print_test("signal_wait_finished", result == 0);
}

/* Test 7: NULL pointer handling */
void test_null_handling(void) {
    const char* err = consumer_producer_put(NULL, "test");
    char* item = consumer_producer_get(NULL);
    int result = consumer_producer_wait_finished(NULL);
    
    consumer_producer_destroy(NULL);  /* Should not crash */
    
    int success = (err != NULL && item == NULL && result == -1);
    print_test("null_pointer_handling", success);
}

int main(void) {
    printf("\n=== CONSUMER-PRODUCER QUEUE UNIT TESTS ===\n\n");
    
    test_init_destroy();
    test_put_get_single();
    test_circular_buffer();
    test_multiple_items();
    test_producer_blocks();
    test_signal_finished();
    test_null_handling();
    
    printf("\n=== RESULTS ===\n");
    printf("Passed: %d/%d\n", passed, test_count);
    
    if (passed == test_count) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed!\n");
        return 1;
    }
}