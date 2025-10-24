#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Global context for this plugin (.so file) */
static plugin_context_t g_context;

/* Write error message to stderr */
void log_error(plugin_context_t* context, const char* message) {
    fprintf(stderr, "[ERROR] [%s] %s\n", context->name, message);
}

/* Log info message (disabled to keep stdout clean) */
void log_info(plugin_context_t* context, const char* message) {
    (void)context;
    (void)message;
    /* Disabled per assignment requirements */
}

/* Worker thread: processes items from queue */
void* plugin_consumer_thread(void* arg) {
    plugin_context_t* context = (plugin_context_t*)arg;
    
    while (1) {
        /* Get next item from queue (blocks if empty) */
        char* input_str = consumer_producer_get(context->queue);
        
        /* Check if this is the shutdown signal */
        if (strcmp(input_str, "<END>") == 0) {
            /* Signal finished BEFORE forwarding <END> to avoid deadlock */
            consumer_producer_signal_finished(context->queue);
            
            /* Forward <END> to next plugin if it exists */
            if (context->next_place_work) {
                context->next_place_work(input_str);
            }
            
            free(input_str);
            break;
        }
        
        /* Apply plugin-specific transformation */
        const char* output_str = context->process_function(input_str);
        free(input_str);
        
        if (context->next_place_work) {
            /* Send to next plugin in chain */
            context->next_place_work(output_str);
            free((void*)output_str);
        } else {
            /* Last plugin - free the output */
            free((void*)output_str);
        }
    }
    
    return NULL;
}

/* Initialize plugin with transformation function and queue size */
const char* common_plugin_init(const char* (*process_function)(const char*),
                              const char* name, int queue_size) {
    
    /* Set up context */
    g_context.name = name;
    g_context.process_function = process_function;
    g_context.next_place_work = NULL;
    g_context.initialized = 0;
    g_context.finished = 0;
    
    /* Create queue */
    g_context.queue = malloc(sizeof(consumer_producer_t));
    if (!g_context.queue) {
        return "Failed to allocate memory for queue";
    }
    
    const char* err = consumer_producer_init(g_context.queue, queue_size);
    if (err) {
        free(g_context.queue);
        return err;
    }
    
    /* Create worker thread */
    if (pthread_create(&g_context.consumer_thread, NULL, 
                      plugin_consumer_thread, &g_context) != 0) {
        consumer_producer_destroy(g_context.queue);
        free(g_context.queue);
        return "Failed to create worker thread";
    }
    
    g_context.initialized = 1;
    return NULL;
}

/* ===== Plugin Interface Functions ===== */

/* Return plugin name */
__attribute__((visibility("default")))
const char* plugin_get_name(void) {
    return g_context.name;
}

/* Cleanup: wait for thread and free resources */
__attribute__((visibility("default")))
const char* plugin_fini(void) {
    if (!g_context.initialized) {
        return NULL;
    }
    
    pthread_join(g_context.consumer_thread, NULL);
    consumer_producer_destroy(g_context.queue);
    free(g_context.queue);
    
    g_context.initialized = 0;
    return NULL;
}

/* Add work to plugin's queue */
__attribute__((visibility("default")))
const char* plugin_place_work(const char* str) {
    if (!g_context.initialized) {
        return "Plugin not initialized";
    }
    return consumer_producer_put(g_context.queue, str);
}

/* Connect to next plugin in chain */
__attribute__((visibility("default")))
void plugin_attach(const char* (*next_place_work)(const char*)) {
    g_context.next_place_work = next_place_work;
}

/* Wait for plugin to finish processing */
__attribute__((visibility("default")))
const char* plugin_wait_finished(void) {
    if (!g_context.initialized) {
        return "Plugin not initialized";
    }
    
    consumer_producer_wait_finished(g_context.queue);
    g_context.finished = 1;
    return NULL;
}