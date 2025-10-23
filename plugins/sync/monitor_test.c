/* * [source: 360] Unit test application for monitor.c
 * This file is required by the new build.sh
 */
#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

monitor_t test_monitor;
int test_flag = 0;

/* Thread function for Test 1: Basic wait/signal */
void* thread_func_wait(void* arg) {
    printf("  [TEST 1] Thread 2: Waiting...\n");
    monitor_wait(&test_monitor);
    printf("  [TEST 1] Thread 2: Signaled and woke up.\n");
    test_flag = 1;
    return NULL;
}

/* Test 1: Basic wait/signal */
void test_basic_wait_signal() {
    printf("[TEST 1] Running: Basic Wait/Signal\n");
    test_flag = 0;
    assert(monitor_init(&test_monitor) == 0);
    
    pthread_t tid;
    pthread_create(&tid, NULL, thread_func_wait, NULL);
    
    // Give the thread time to enter monitor_wait
    sleep(1);
    
    printf("  [TEST 1] Thread 1: Signaling...\n");
    monitor_signal(&test_monitor);
    
    pthread_join(tid, NULL);
    assert(test_flag == 1);
    
    monitor_destroy(&test_monitor);
    printf("[TEST 1] PASS\n\n");
}

/* Thread function for Test 2: Stateful signal-before-wait */
void* thread_func_stateful(void* arg) {
    printf("  [TEST 2] Thread 2: Calling wait (should not block)...\n");
    monitor_wait(&test_monitor);
    printf("  [TEST 2] Thread 2: Wait returned immediately.\n");
    test_flag = 2;
    return NULL;
}

/* [source: 306-319] Test 2: Stateful signal-before-wait (the "lost signal" problem) */
void test_stateful_signal_before_wait() {
    printf("[TEST 2] Running: Stateful Signal-Before-Wait\n");
    test_flag = 0;
    assert(monitor_init(&test_monitor) == 0);
    
    printf("  [TEST 2] Thread 1: Signaling *before* thread 2 waits...\n");
    monitor_signal(&test_monitor); // Signal *first*
    
    pthread_t tid;
    pthread_create(&tid, NULL, thread_func_stateful, NULL);
    
    pthread_join(tid, NULL);
    assert(test_flag == 2); // Check that the thread ran and set the flag
    
    monitor_destroy(&test_monitor);
    printf("[TEST 2] PASS\n\n");
}


int main() {
    printf("--- Running Monitor Unit Tests ---\n\n");
    
    test_basic_wait_signal();
    test_stateful_signal_before_wait();
    
    printf("--- All Monitor Tests Passed ---\n");
    return 0;
}
