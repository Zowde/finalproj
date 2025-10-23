/* */
#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include "plugin_sdk.h"
#include "sync/consumer_producer.h"
#include <pthread.h>

/* For strdup */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

/**
 * Plugin context structure
 */
typedef struct /* */
{
    const char* name;          /* */
    consumer_producer_t* queue; /* */
    pthread_t consumer_thread; /* */
    
    /* Next plugin's place_work function */
    const char* (*next_place_work) (const char*); 
    
    /* Plugin-specific processing function */
    const char* (*process_function) (const char*); 
    
    int initialized; /* */
    int finished;    /* */
} plugin_context_t; /* */

/**
 * Generic consumer thread function
 * This function runs in a separate thread and processes items from the queue
 * @param arg Pointer to plugin_context_t
 * @return NULL
 */
void* plugin_consumer_thread(void* arg); /* */

/**
 * Print error message in the format [ERROR] [Plugin Name] message
 * @param context Plugin context
 * @param message Error message
 */
void log_error(plugin_context_t* context, const char* message); /* */

/**
 * Print info message in the format [INFO] [Plugin Name] message
 * @param context Plugin context
 * @param message Info message
 */
void log_info(plugin_context_t* context, const char* message); /* */

/**
 * Get the plugin's name
 * @return The plugin's name
 */
/* Mark as exported for visibility */
__attribute__((visibility("default"))) 
const char* plugin_get_name(void); /* */

/**
 * Initialize the common plugin infrastructure
 * @param process_function Plugin-specific processing function
 * @param name Plugin name
 * @param queue_size Maximum number of items that can be queued
 * @return NULL on success, error message on failure
 */
const char* common_plugin_init(const char* (*process_function) (const char*), /* */
                             const char* name, int queue_size); /* */

/**
 * Initialize the plugin (to be implemented by each plugin)
 * @param queue_size Maximum number of items
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default"))) /* */
const char* plugin_init(int queue_size); /* */

/**
 * Finalize the plugin drain queue and terminate thread gracefully
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default"))) /* */
const char* plugin_fini(void); /* */

/**
 * Place work (a string) into the plugin's queue
 * @param str The string to process
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default"))) /* */
const char* plugin_place_work(const char* str); /* */

/**
 * Attach this plugin to the next plugin in the chain
 * @param next_place_work Function pointer to the next plugin's place_work
 */
__attribute__((visibility("default"))) /* */
void plugin_attach(const char* (*next_place_work) (const char*)); /* */

/**
 * Wait until the plugin has finished processing
 * This is a blocking function
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default"))) /* */
const char* plugin_wait_finished(void); /* */


#endif // PLUGIN_COMMON_H