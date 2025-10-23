#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "plugins/plugin_sdk.h"

/**
 * Function pointer types from plugin SDK
 */
typedef const char* (*plugin_init_func_t)(int queue_size);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*place_work_func_t)(const char*);
typedef void (*plugin_attach_func_t)(const char* (*next_place_work)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);
typedef const char* (*plugin_get_name_func_t)(void);

/**
 * Plugin handle structure - stores all function pointers and metadata
 */
typedef struct {
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    plugin_get_name_func_t get_name;
    char* name;
    void* handle;
} plugin_handle_t;

/**
 * Print usage information
 */
void print_usage(void) {
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n");
    printf("Arguments:\n");
    printf("  queue_size Maximum number of items in each plugin's queue\n");
    printf("  plugin1..N Names of plugins to load (without .so extension)\n");
    printf("Available plugins:\n");
    printf("  logger - Logs all strings that pass through\n");
    printf("  typewriter - Simulates typewriter effect with delays\n");
    printf("  uppercaser - Converts strings to uppercase\n");
    printf("  rotator - Move every character to the right. Last character moves to the beginning.\n");
    printf("  flipper - Reverses the order of characters\n");
    printf("  expander - Expands each character with spaces\n");
    printf("Example:\n");
    printf("  ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo 'hello' | ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo '<END>' | ./analyzer 20 uppercaser rotator logger\n");
}

/**
 * Step 1: Parse command-line arguments
 */
int parse_arguments(int argc, char* argv[], int* queue_size, char*** plugin_names, int* plugin_count) {
    if (argc < 3) {
        fprintf(stderr, "Error: Missing arguments\n");
        print_usage();
        return -1;
    }
    
    /* Parse queue size */
    *queue_size = atoi(argv[1]);
    if (*queue_size <= 0) {
        fprintf(stderr, "Error: queue_size must be a positive integer\n");
        print_usage();
        return -1;
    }
    
    /* Get plugin names */
    *plugin_count = argc - 2;
    *plugin_names = &argv[2];
    
    return 0;
}

/**
 * Step 2: Load plugin shared objects and extract interfaces
 */
int load_plugins(char** plugin_names, int plugin_count, int queue_size, 
                 plugin_handle_t** plugins_out) {
    (void)queue_size;  /* Suppress unused parameter warning - queue_size passed to each plugin via init() */
    /* Allocate array of plugin handles */
    *plugins_out = (plugin_handle_t*)malloc(plugin_count * sizeof(plugin_handle_t));
    if (*plugins_out == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }
    
    for (int i = 0; i < plugin_count; i++) {
        plugin_handle_t* plugin = &(*plugins_out)[i];
        
        /* Construct filename */
        char filename[256];
        snprintf(filename, sizeof(filename), "./output/%s.so", plugin_names[i]);
        
        /* Load plugin */
        plugin->handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
        if (plugin->handle == NULL) {
            fprintf(stderr, "Error: Failed to load plugin %s: %s\n", plugin_names[i], dlerror());
            print_usage();
            return -1;
        }
        
        /* Get function pointers */
        plugin->init = (plugin_init_func_t)dlsym(plugin->handle, "plugin_init");
        if (plugin->init == NULL) {
            fprintf(stderr, "Error: Failed to find plugin_init in %s: %s\n", plugin_names[i], dlerror());
            dlclose(plugin->handle);
            print_usage();
            return -1;
        }
        
        plugin->fini = (plugin_fini_func_t)dlsym(plugin->handle, "plugin_fini");
        if (plugin->fini == NULL) {
            fprintf(stderr, "Error: Failed to find plugin_fini in %s: %s\n", plugin_names[i], dlerror());
            dlclose(plugin->handle);
            print_usage();
            return -1;
        }
        
        plugin->place_work = (place_work_func_t)dlsym(plugin->handle, "plugin_place_work");
        if (plugin->place_work == NULL) {
            fprintf(stderr, "Error: Failed to find plugin_place_work in %s: %s\n", plugin_names[i], dlerror());
            dlclose(plugin->handle);
            print_usage();
            return -1;
        }
        
        plugin->attach = (plugin_attach_func_t)dlsym(plugin->handle, "plugin_attach");
        if (plugin->attach == NULL) {
            fprintf(stderr, "Error: Failed to find plugin_attach in %s: %s\n", plugin_names[i], dlerror());
            dlclose(plugin->handle);
            print_usage();
            return -1;
        }
        
        plugin->wait_finished = (plugin_wait_finished_func_t)dlsym(plugin->handle, "plugin_wait_finished");
        if (plugin->wait_finished == NULL) {
            fprintf(stderr, "Error: Failed to find plugin_wait_finished in %s: %s\n", plugin_names[i], dlerror());
            dlclose(plugin->handle);
            print_usage();
            return -1;
        }
        
        plugin->get_name = (plugin_get_name_func_t)dlsym(plugin->handle, "plugin_get_name");
        if (plugin->get_name == NULL) {
            fprintf(stderr, "Error: Failed to find plugin_get_name in %s: %s\n", plugin_names[i], dlerror());
            dlclose(plugin->handle);
            print_usage();
            return -1;
        }
        
        plugin->name = (char*)plugin->get_name();
    }
    
    return 0;
}

/**
 * Step 3: Initialize plugins
 */
int initialize_plugins(plugin_handle_t* plugins, int plugin_count, int queue_size) {
    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].init(queue_size);
        if (error != NULL) {
            fprintf(stderr, "Error: Failed to initialize plugin %s: %s\n", plugins[i].name, error);
            return -1;
        }
    }
    return 0;
}

/**
 * Step 4: Attach plugins together (wire the pipeline)
 */
void attach_plugins(plugin_handle_t* plugins, int plugin_count) {
    for (int i = 0; i < plugin_count - 1; i++) {
        /* Attach plugin i to plugin i+1 */
        plugins[i].attach(plugins[i + 1].place_work);
    }
    /* Last plugin is not attached to anything */
}

/**
 * Step 5: Read input from STDIN and process
 */
int read_and_process_input(plugin_handle_t* first_plugin) {
    char line[1024];
    
    while (fgets(line, sizeof(line), stdin) != NULL) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        /* Send to first plugin */
        const char* error = first_plugin->place_work(line);
        if (error != NULL) {
            fprintf(stderr, "Error: Failed to place work: %s\n", error);
            return -1;
        }
        
        /* Check if this was the END signal */
        if (strcmp(line, "<END>") == 0) {
            break;
        }
    }
    
    return 0;
}

/**
 * Step 6: Wait for all plugins to finish
 */
int wait_for_plugins(plugin_handle_t* plugins, int plugin_count) {
    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].wait_finished();
        if (error != NULL) {
            fprintf(stderr, "Error: Failed waiting for plugin %s: %s\n", plugins[i].name, error);
            return -1;
        }
    }
    return 0;
}

/**
 * Step 7: Cleanup
 */
void cleanup_plugins(plugin_handle_t* plugins, int plugin_count) {
    for (int i = 0; i < plugin_count; i++) {
        /* Call plugin finalize */
        const char* error = plugins[i].fini();
        if (error != NULL) {
            fprintf(stderr, "Error: Failed to finalize plugin %s: %s\n", plugins[i].name, error);
        }
        
        /* Unload plugin */
        if (plugins[i].handle != NULL) {
            dlclose(plugins[i].handle);
        }
    }
    
    /* Free plugins array */
    free(plugins);
}

/**
 * Main application
 */
int main(int argc, char* argv[]) {
    int queue_size = 0;
    char** plugin_names = NULL;
    int plugin_count = 0;
    plugin_handle_t* plugins = NULL;
    
    /* Step 1: Parse command-line arguments */
    if (parse_arguments(argc, argv, &queue_size, &plugin_names, &plugin_count) != 0) {
        return 1;
    }
    
    /* Step 2: Load plugin shared objects */
    if (load_plugins(plugin_names, plugin_count, queue_size, &plugins) != 0) {
        return 1;
    }
    
    /* Step 3: Initialize plugins */
    if (initialize_plugins(plugins, plugin_count, queue_size) != 0) {
        cleanup_plugins(plugins, plugin_count);
        return 2;
    }
    
    /* Step 4: Attach plugins together */
    attach_plugins(plugins, plugin_count);
    
    /* Step 5: Read input from STDIN and process */
    if (read_and_process_input(&plugins[0]) != 0) {
        cleanup_plugins(plugins, plugin_count);
        return 1;
    }
    
    /* Step 6: Wait for all plugins to finish */
    if (wait_for_plugins(plugins, plugin_count) != 0) {
        cleanup_plugins(plugins, plugin_count);
        return 1;
    }
    
    /* Step 7: Cleanup and finalize */
    cleanup_plugins(plugins, plugin_count);
    
    /* Step 8: Print final message */
    printf("Pipeline shutdown complete\n");
    
    return 0;
}