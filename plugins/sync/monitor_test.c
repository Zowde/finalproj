#include "monitor.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Test counter */
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

/* Test 1: Basic initialization and destruction */
void test_init_destroy(void) {
    monitor_t mon;
    int result = monitor_init(&mon);
    int destroy_ok = (result == 0);
    monitor_destroy(&mon);
    print_test("init_destroy", destroy_ok);
}

/* Test 2: Signal before wait (key test - solves race condition) */
void test_signal_before_wait(void) {
    monitor_t mon;
    monitor_init(&mon);
    
    /* Signal BEFORE wait */
    monitor_signal(&mon);
    
    /* Now wait should NOT block - it should see the signal flag */
    int result = monitor_wait(&mon);
    
    monitor_destroy(&mon);
    print_test("signal_before_wait", result == 0);
}

/* Test 3: Reset clears the signal */
void test_reset(void) {
    monitor_t mon;
    monitor_init(&mon);
    
    monitor_signal(&mon);
    monitor_reset(&mon);
    
    /* After reset, signaled should be 0 */
    print_test("reset", mon.signaled == 0);
    
    monitor_destroy(&mon);
}

/* Shared state for thread test */
static int thread_test_flag = 0;
static monitor_t test_mon;

void* signaler_thread(void* arg) {
    (void)arg;  /* Suppress unused parameter warning */
    usleep(100000);  /* Wait 100ms */
    monitor_signal(&test_mon);
    return NULL;
}

/* Test 4: Wait blocks until signal from another thread */
void test_wait_blocks(void) {
    monitor_init(&test_mon);
    thread_test_flag = 0;
    
    pthread_t tid;
    pthread_create(&tid, NULL, signaler_thread, NULL);
    
    /* This should block until signaler_thread signals */
    int result = monitor_wait(&test_mon);
    
    pthread_join(tid, NULL);
    monitor_destroy(&test_mon);
    
    print_test("wait_blocks_until_signal", result == 0);
}

/* Test 5: Multiple signals */
void test_multiple_signals(void) {
    monitor_t mon;
    monitor_init(&mon);
    
    monitor_signal(&mon);
    int wait1 = monitor_wait(&mon);
    
    /* After waiting, signaled is still set (manual reset) */
    /* Signal again */
    monitor_signal(&mon);
    int wait2 = monitor_wait(&mon);
    
    monitor_destroy(&mon);
    print_test("multiple_signals", wait1 == 0 && wait2 == 0);
}

/* Test 6: NULL pointer handling */
void test_null_handling(void) {
    /* These should not crash */
    monitor_destroy(NULL);
    monitor_signal(NULL);
    monitor_reset(NULL);
    int result = monitor_wait(NULL);
    
    print_test("null_pointer_handling", result == -1);
}

int main(void) {
    printf("\n=== MONITOR UNIT TESTS ===\n\n");
    
    test_init_destroy();
    test_signal_before_wait();
    test_reset();
    test_wait_blocks();
    test_multiple_signals();
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