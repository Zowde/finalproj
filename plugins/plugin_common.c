#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Global plugin context - shared across all plugin functions
 * Since we're building .so files with each plugin in isolation,
 * each .so file will have its own global context
 */
static plugin_context_t* g_plugin_context = NULL;

/**
 * Generic consumer thread function
 * Runs in a separate thread, processing items from the queue until <END>
 */
void* plugin_consumer_thread(void* arg) {
    plugin_context_t* context = (plugin_context_t*)arg;
    
    if (context == NULL) {
        return NULL;
    }
    
    /* Process items from queue until <END> */
    while (1) {
        char* item = consumer_producer_get(context->queue);
        
        if (item == NULL) {
            break;  /* Error getting item */
        }
        
        /* Check if this is the END signal */
        if (strcmp(item, "<END>") == 0) {
            /* Pass <END> to next plugin if not last */
            if (context->next_place_work != NULL) {
                context->next_place_work(item);
            }
            free(item);
            break;  /* Exit thread */
        }
        
        /* Call plugin-specific transformation */
        const char* result = context->process_function(item);
        
        free(item);  /* Free the input */
        
        if (result == NULL) {
            continue;  /* Skip if transformation failed */
        }
        
        /* If this is not the last plugin, pass result to next plugin */
        if (context->next_place_work != NULL) {
            context->next_place_work(result);
        } else {
            /* This is the last plugin, free the result */
            free((char*)result);
        }
    }
    
    /* Mark as finished */
    context->finished = 1;
    consumer_producer_signal_finished(context->queue);
    
    return NULL;
}

/**
 * Print error message
 */
void log_error(plugin_context_t* context, const char* message) {
    if (context == NULL || message == NULL) {
        return;
    }
    fprintf(stderr, "[ERROR][%s] - %s\n", context->name, message);
}

/**
 * Print info message (suppressed in final version per assignment)
 */
void log_info(plugin_context_t* context, const char* message) {
    (void)context;  /* Unused in final version */
    (void)message;  /* Unused in final version */
    /* Info logging disabled to keep STDOUT clean as per assignment requirements */
}

/**
 * Initialize the common plugin infrastructure
 * Each plugin calls this from its plugin_init()
 */
const char* common_plugin_init(process_func_t process_function, const char* name, int queue_size) {
    if (g_plugin_context != NULL) {
        return "Plugin already initialized";
    }
    
    if (process_function == NULL || name == NULL || queue_size <= 0) {
        return "Invalid parameters";
    }
    
    /* Allocate plugin context */
    g_plugin_context = (plugin_context_t*)malloc(sizeof(plugin_context_t));
    if (g_plugin_context == NULL) {
        return "Memory allocation failed for plugin context";
    }
    
    /* Initialize context */
    g_plugin_context->name = name;
    g_plugin_context->process_function = process_function;
    g_plugin_context->next_place_work = NULL;
    g_plugin_context->initialized = 0;
    g_plugin_context->finished = 0;
    
    /* Allocate and initialize queue */
    g_plugin_context->queue = (consumer_producer_t*)malloc(sizeof(consumer_producer_t));
    if (g_plugin_context->queue == NULL) {
        free(g_plugin_context);
        g_plugin_context = NULL;
        return "Memory allocation failed for queue";
    }
    
    const char* queue_err = consumer_producer_init(g_plugin_context->queue, queue_size);
    if (queue_err != NULL) {
        free(g_plugin_context->queue);
        free(g_plugin_context);
        g_plugin_context = NULL;
        return queue_err;
    }
    
    /* Create consumer thread */
    if (pthread_create(&g_plugin_context->consumer_thread, NULL, 
                       plugin_consumer_thread, g_plugin_context) != 0) {
        consumer_producer_destroy(g_plugin_context->queue);
        free(g_plugin_context->queue);
        free(g_plugin_context);
        g_plugin_context = NULL;
        return "Failed to create consumer thread";
    }
    
    g_plugin_context->initialized = 1;
    return NULL;  /* Success */
}

/**
 * Plugin SDK interface functions
 * These are called by the main application via dlsym
 */

const char* plugin_get_name(void) {
    if (g_plugin_context == NULL) {
        return "Unknown";
    }
    return g_plugin_context->name;
}

const char* plugin_fini(void) {
    if (g_plugin_context == NULL) {
        return "Plugin not initialized";
    }
    
    /* Wait for consumer thread to finish */
    if (pthread_join(g_plugin_context->consumer_thread, NULL) != 0) {
        return "Failed to join consumer thread";
    }
    
    /* Clean up queue */
    consumer_producer_destroy(g_plugin_context->queue);
    free(g_plugin_context->queue);
    
    /* Free context */
    free(g_plugin_context);
    g_plugin_context = NULL;
    
    return NULL;  /* Success */
}

const char* plugin_place_work(const char* str) {
    if (g_plugin_context == NULL) {
        return "Plugin not initialized";
    }
    
    if (str == NULL) {
        return "String is NULL";
    }
    
    return consumer_producer_put(g_plugin_context->queue, str);
}

void plugin_attach(const char* (*next_place_work)(const char*)) {
    if (g_plugin_context == NULL) {
        return;
    }
    
    g_plugin_context->next_place_work = next_place_work;
}

const char* plugin_wait_finished(void) {
    if (g_plugin_context == NULL) {
        return "Plugin not initialized";
    }
    
    if (consumer_producer_wait_finished(g_plugin_context->queue) != 0) {
        return "Wait finished failed";
    }
    
    return NULL;  /* Success */
}