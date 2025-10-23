#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include <pthread.h>
#include "sync/consumer_producer.h"

/**
 * Common SDK structures and functions for plugin implementation
 * This layer handles all the threading, queuing, and synchronization
 * Plugin authors only need to implement their transformation logic
 */

/* Function pointer types */
typedef const char* (*process_func_t)(const char*);
typedef const char* (*place_work_func_t)(const char*);

/**
 * Plugin context structure
 * Manages the plugin's lifecycle, queue, thread, and connections
 */
typedef struct {
    const char* name;                           /* Plugin name (for diagnostics) */
    consumer_producer_t* queue;                 /* Input queue */
    pthread_t consumer_thread;                  /* Consumer thread handle */
    place_work_func_t next_place_work;          /* Next plugin's place_work function */
    process_func_t process_function;            /* Plugin-specific processing function */
    int initialized;                            /* Initialization flag */
    int finished;                               /* Finished processing flag */
} plugin_context_t;

/**
 * Generic consumer thread function
 * This function runs in a separate thread and processes items from the queue
 * Calls the plugin-specific process_function for each item
 * @param arg Pointer to plugin_context_t
 * @return NULL
 */
void* plugin_consumer_thread(void* arg);

/**
 * Print error message in the format [ERROR][Plugin Name] - message
 * @param context Plugin context
 * @param message Error message
 */
void log_error(plugin_context_t* context, const char* message);

/**
 * Print info message in the format [INFO][Plugin Name] - message
 * @param context Plugin context
 * @param message Info message
 */
void log_info(plugin_context_t* context, const char* message);

/**
 * Get the plugin's name (exported by each plugin)
 * @return The plugin's name (should not be modified or freed)
 */
__attribute__((visibility("default")))
const char* plugin_get_name(void);

/**
 * Initialize the common plugin infrastructure with the specified queue size
 * This sets up the queue, creates the consumer thread, and initializes the context
 * @param process_function Plugin-specific processing function
 * @param name Plugin name
 * @param queue_size Maximum number of items that can be queued
 * @return NULL on success, error message on failure
 */
const char* common_plugin_init(process_func_t process_function, const char* name, int queue_size);

/**
 * Initialize the plugin with the specified queue size
 * Calls common_plugin_init internally
 * Each plugin must implement this, calling common_plugin_init
 * @param queue_size Maximum number of items that can be queued
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size);

/**
 * Finalize the plugin - drain queue and terminate thread gracefully
 * Joins the consumer thread and cleans up resources
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_fini(void);

/**
 * Place work (a string) into the plugin's queue
 * @param str The string to process (queue takes ownership)
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_place_work(const char* str);

/**
 * Attach this plugin to the next plugin in the chain
 * Stores the next plugin's place_work function pointer
 * @param next_place_work Function pointer to the next plugin's place_work function
 */
__attribute__((visibility("default")))
void plugin_attach(const char* (*next_place_work)(const char*));

/**
 * Wait until the plugin has finished processing all work and is ready to shutdown
 * Blocks until the finished signal is received
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_wait_finished(void);

#endif /* PLUGIN_COMMON_H */