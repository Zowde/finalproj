/* */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>      /* For dlopen, dlsym, dlclose, dlerror */
#include "plugins/plugin_sdk.h"
#include <fcntl.h>      /* For open */
#include <unistd.h>     /* For read, write, close, unlink */
#include <sys/stat.h>   /* For fstat, stat */

/* Function pointer typedefs for dlsym */
typedef const char* (*plugin_init_func_t)(int);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*plugin_place_work_func_t)(const char*);
typedef void (*plugin_attach_func_t)(const char* (*)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);

/**
 * Structure to hold plugin handles and function pointers
 */
typedef struct {
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    plugin_place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    char* name;         /* Plugin name, e.g., "rotator" */
    void* handle;       /* Handle from dlopen() */
    char* so_path;      /* Path to the .so file, e.g., "output/rotator.1.so" */
} plugin_handle_t;

/**
 * Prints the usage message to stdout.
 */
void print_usage() {
    /* Print as a single string to avoid buffering issues on exit */
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n"
           "Arguments:\n"
           "  queue_size   Maximum number of items in each plugin's queue\n"
           "  plugin1..N   Names of plugins to load (without so extension)\n"
           "Available plugins:\n"
           "  logger       Logs all strings that pass through\n"
           "  typewriter   Simulates typewriter effect with delays\n"
           "  uppercaser   Converts strings to uppercase\n"
           "  rotator      Move every character to the right. Last character moves to the beginning.\n"
           "  flipper      Reverses the order of characters\n"
           "  expander     Expands each character with spaces\n"
           "Example:\n"
           "  ./analyzer 20 uppercaser rotator logger\n");
}

/**
 * Helper function to copy a file using a read/write loop.
 * This is more portable than sendfile, especially for WSL.
 * Returns 0 on success, -1 on failure.
 */
int copy_file(const char* src_path, const char* dest_path) {
    int src_fd, dest_fd;
    ssize_t n_read;
    char buf[4096];
    int ret = -1; // Assume failure

    src_fd = open(src_path, O_RDONLY);
    if (src_fd == -1) {
        perror("copy_file open src");
        return -1;
    }

    struct stat stat_buf;
    if (fstat(src_fd, &stat_buf) == -1) {
        perror("copy_file fstat");
        close(src_fd);
        return -1;
    }

    dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode);
    if (dest_fd == -1) {
        perror("copy_file open dest");
        close(src_fd);
        return -1;
    }

    while ((n_read = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dest_fd, buf, n_read) != n_read) {
            perror("copy_file write");
            goto cleanup; // Go to cleanup
        }
    }

    if (n_read == 0) {
        ret = 0; // Success
    } else {
        perror("copy_file read");
    }

cleanup:
    close(src_fd);
    close(dest_fd);
    if (ret != 0) {
        unlink(dest_path); // Remove partial file on failure
    }
    return ret;
}


int main(int argc, char* argv[]) {
    
    /* --- Step 1: Parse Command-Line Arguments --- */
    if (argc < 3) {
        fprintf(stderr, "Error: Missing arguments.\n");
        print_usage();
        fflush(stdout); /* Flush stdout buffer before exit */
        exit(1);
    }
    
    int queue_size = atoi(argv[1]);
    if (queue_size <= 0) {
        fprintf(stderr, "Error: queue_size must be a positive integer.\n");
        print_usage();
        fflush(stdout); /* Flush stdout buffer before exit */
        exit(1);
    }
    
    int num_plugins = argc - 2;
    char** plugin_names = &argv[2];
    
    plugin_handle_t* plugins = calloc(num_plugins, sizeof(plugin_handle_t));
    if (!plugins) {
        fprintf(stderr, "Error: Failed to allocate memory for plugin handles.\n");
        exit(1);
    }
    
    /* --- Step 2: Load Plugin Shared Objects --- */
    for (int i = 0; i < num_plugins; i++) {
        char src_path[256];
        char final_so_path[256];
        
        snprintf(src_path, sizeof(src_path), "output/%s.so", plugin_names[i]);

        int is_reused = 0;
        for (int j = 0; j < i; j++) {
            if (strcmp(plugin_names[j], plugin_names[i]) == 0) {
                is_reused = 1;
                break;
            }
        }

        if (is_reused) {
            snprintf(final_so_path, sizeof(final_so_path), "output/%s.%d.so", plugin_names[i], i);
            if (copy_file(src_path, final_so_path) != 0) {
                fprintf(stderr, "Error: Failed to create copy of plugin %s\n", src_path);
                print_usage();
                fflush(stdout);
                /* Cleanup all previously loaded plugins */
                for (int j = 0; j < i; j++) {
                    if (plugins[j].handle) dlclose(plugins[j].handle);
                    if (plugins[j].so_path) {
                        char orig_path[256];
                        snprintf(orig_path, sizeof(orig_path), "output/%s.so", plugins[j].name);
                        if (strcmp(orig_path, plugins[j].so_path) != 0) {
                            unlink(plugins[j].so_path);
                        }
                        free(plugins[j].so_path);
                    }
                    free(plugins[j].name);
                }
                free(plugins);
                exit(1);
            }
        } else {
            strncpy(final_so_path, src_path, sizeof(final_so_path));
            final_so_path[sizeof(final_so_path) - 1] = '\0';
        }

        plugins[i].so_path = strdup(final_so_path);
        if (!plugins[i].so_path) {
            fprintf(stderr, "Error: strdup failed for plugin path.\n");
            exit(1);
        }
        
        plugins[i].handle = dlopen(final_so_path, RTLD_NOW | RTLD_LOCAL);
        if (!plugins[i].handle) {
            fprintf(stderr, "Error loading plugin %s: %s\n", final_so_path, dlerror());
            print_usage();
            fflush(stdout);
            /* Cleanup this and all previously loaded plugins */
            for (int j = 0; j <= i; j++) {
                if(plugins[j].handle) dlclose(plugins[j].handle);
                if (plugins[j].so_path) {
                    char orig_path[256];
                    snprintf(orig_path, sizeof(orig_path), "output/%s.so", plugin_names[j]);
                    if (strcmp(orig_path, plugins[j].so_path) != 0) {
                        unlink(plugins[j].so_path);
                    }
                    free(plugins[j].so_path);
                }
                free(plugins[j].name);
            }
            free(plugins);
            exit(1);
        }
        
        /* Resolve all required functions */
        plugins[i].init = (plugin_init_func_t)dlsym(plugins[i].handle, "plugin_init");
        plugins[i].fini = (plugin_fini_func_t)dlsym(plugins[i].handle, "plugin_fini");
        plugins[i].place_work = (plugin_place_work_func_t)dlsym(plugins[i].handle, "plugin_place_work");
        plugins[i].attach = (plugin_attach_func_t)dlsym(plugins[i].handle, "plugin_attach");
        plugins[i].wait_finished = (plugin_wait_finished_func_t)dlsym(plugins[i].handle, "plugin_wait_finished");
        
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            fprintf(stderr, "Error resolving symbols in %s: %s\n", final_so_path, dlsym_error);
            print_usage();
            fflush(stdout);
            /* Cleanup this and all previously loaded plugins */
            for (int j = 0; j <= i; j++) {
                if(plugins[j].handle) dlclose(plugins[j].handle);
                if (plugins[j].so_path) {
                    char orig_path[256];
                    snprintf(orig_path, sizeof(orig_path), "output/%s.so", plugin_names[j]);
                    if (strcmp(orig_path, plugins[j].so_path) != 0) {
                        unlink(plugins[j].so_path);
                    }
                    free(plugins[j].so_path);
                }
                free(plugins[j].name);
            }
            free(plugins);
            exit(1);
        }
        
        plugins[i].name = strdup(plugin_names[i]);
    }
    
    /* --- Step 3: Initialize Plugins --- */
    for (int i = 0; i < num_plugins; i++) {
        const char* err = plugins[i].init(queue_size);
        if (err) {
            fprintf(stderr, "Error initializing plugin %s: %s\n", plugins[i].name, err);
            /* Cleanup all plugins */
            for (int j = 0; j < num_plugins; j++) {
                if (plugins[j].handle) dlclose(plugins[j].handle);
                if (plugins[j].so_path) {
                    char orig_path[256];
                    snprintf(orig_path, sizeof(orig_path), "output/%s.so", plugins[j].name);
                    if (strcmp(orig_path, plugins[j].so_path) != 0) {
                        unlink(plugins[j].so_path);
                    }
                    free(plugins[j].so_path);
                }
                free(plugins[j].name);
            }
            free(plugins);
            exit(2);
        }
    }
    
    /* --- Step 4: Attach Plugins Together --- */
    for (int i = 0; i < num_plugins - 1; i++) {
        /* Attach plugin[i] to plugin[i+1]'s place_work function */
        plugins[i].attach(plugins[i+1].place_work);
    }
    /* The last plugin is not attached to anything */
    
    /* --- Step 5: Read Input from STDIN --- */
    char line[1026]; /* 1024 chars + '\n' + '\0' */
    int sent_end = 0; /* Flag to track if <END> was sent */

    while (fgets(line, sizeof(line), stdin)) {
        
        line[strcspn(line, "\n")] = 0; /* Remove trailing newline */
        
        /* Send the line to the first plugin */
        plugins[0].place_work(line);
        
        /* If <END> is received, break the loop */
        if (strcmp(line, "<END>") == 0) {
            sent_end = 1; /* Mark that we sent <END> */
            break;
        }
    }

    /* * FIX: Handle immediate EOF (Test 16).
     * If the loop ended but we never sent <END>, we must send it now
     * to trigger the graceful shutdown chain.
     */
    if (!sent_end) {
        plugins[0].place_work("<END>");
    }
    
    /* --- Step 6: Wait for Plugins to Finish --- */
    for (int i = 0; i < num_plugins; i++) {
        plugins[i].wait_finished();
    }
    
    /* --- Step 7: Cleanup --- */
    for (int i = 0; i < num_plugins; i++) {
        plugins[i].fini();
        dlclose(plugins[i].handle);
        free(plugins[i].name);
        
        /* Remove the copied .so file if we made one */
        if (plugins[i].so_path) {
            char src_path[256];
            snprintf(src_path, sizeof(src_path), "output/%s.so", plugin_names[i]);
            if (strcmp(src_path, plugins[i].so_path) != 0) {
                unlink(plugins[i].so_path);
            }
            free(plugins[i].so_path);
        }
    }
    free(plugins);
    
    /* --- Step 8: Finalize --- */
    printf("Pipeline shutdown complete\n");
    
    exit(0);
}

