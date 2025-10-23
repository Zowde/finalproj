/* */
#include "plugin_common.h"

/* Use a single global context for this plugin instance (.so file) */
static plugin_context_t g_context;

void log_error(plugin_context_t* context, const char* message) { /* */
    /* Errors must be written to STDERR */
    fprintf(stderr, "[ERROR] [%s] %s\n", context->name, message); /* */
}

void log_info(plugin_context_t* context, const char* message) { /* */
    /* Per instructions, non-error logs should not go to STDOUT. */
    /* We can write to stderr for debugging or disable them. */
    // fprintf(stderr, "[INFO] [%s] %s\n", context->name, message); /* */
}

void* plugin_consumer_thread(void* arg) { /* */
    plugin_context_t* context = (plugin_context_t*)arg; /* */
    
    while (1) {
        /* Get work from the queue (blocks if empty) */
        char* input_str = consumer_producer_get(context->queue); 
        
        /* Check for the shutdown signal */
        if (strcmp(input_str, "<END>") == 0) {
            /* If there's a next plugin, forward the <END> signal */
            if (context->next_place_work) { /* */
                context->next_place_work(input_str);
            }
            
            /* Signal that this plugin has finished processing */
            consumer_producer_signal_finished(context->queue);
            free(input_str); /* Free the <END> string */
            break; /* Exit the thread loop */
        }
        
        /* Process the string using the plugin-specific function */
        const char* processed_str = context->process_function(input_str);
        
        /* The original input string is no longer needed */
        free(input_str);
        
        if (context->next_place_work) { /* */
            /* Pass the result to the next plugin */
            context->next_place_work(processed_str);
            
            /* The next plugin's queue (put) will strdup it, so we free our copy */
            free((void*)processed_str);
        } else {
            /* This is the last plugin in the chain. It must free the final string. */
            free((void*)processed_str);
        }
    }
    
    return NULL; /* */
}

const char* common_plugin_init(const char* (*process_function) (const char*), /* */
                             const char* name, int queue_size) { /* */
    
    g_context.name = name; /* */
    g_context.process_function = process_function; /* */
    g_context.next_place_work = NULL; /* */
    g_context.initialized = 0; /* */
    g_context.finished = 0; /* */
    
    g_context.queue = malloc(sizeof(consumer_producer_t)); /* */
    if (!g_context.queue) {
        return "Failed to allocate memory for plugin queue.";
    }
    
    /* Initialize the queue */
    const char* err = consumer_producer_init(g_context.queue, queue_size); /* */
    if (err) {
        free(g_context.queue);
        return err;
    }
    
    /* Launch the worker thread */
    if (pthread_create(&g_context.consumer_thread, NULL, plugin_consumer_thread, &g_context) != 0) {
        consumer_producer_destroy(g_context.queue);
        free(g_context.queue);
        return "Failed to create plugin consumer thread.";
    }
    
    g_context.initialized = 1; /* */
    return NULL; /* */
}

/* --- Default Interface Implementations --- */

const char* plugin_get_name(void) { /* */
    return g_context.name; /* */
}

const char* plugin_fini(void) { /* */
    if (!g_context.initialized) { /* */
        return NULL;
    }
    
    /* Wait for the consumer thread to exit */
    pthread_join(g_context.consumer_thread, NULL); /* */
    
    /* Clean up queue resources */
    consumer_producer_destroy(g_context.queue);
    free(g_context.queue); /* */
    
    g_context.initialized = 0;
    return NULL; /* */
}

const char* plugin_place_work(const char* str) { /* */
    if (!g_context.initialized) {
        return "Plugin not initialized.";
    }
    /* Place the string into this plugin's input queue */
    return consumer_producer_put(g_context.queue, str); /* */
}

void plugin_attach(const char* (*next_place_work) (const char*)) { /* */
    /* Store the function pointer to the next plugin's place_work */
    g_context.next_place_work = next_place_work; /* */
}

const char* plugin_wait_finished(void) { /* */
    if (!g_context.initialized) {
        return "Plugin not initialized.";
    }
    
    /* Block until the consumer thread signals it's finished */
    consumer_producer_wait_finished(g_context.queue);
    
    g_context.finished = 1; /* */
    return NULL; /* */
}