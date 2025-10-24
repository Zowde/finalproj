#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "plugins/plugin_sdk.h"

/* Function pointer types for dlsym */
typedef const char* (*plugin_init_func_t)(int);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*plugin_place_work_func_t)(const char*);
typedef void (*plugin_attach_func_t)(const char* (*)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);

/* Store loaded plugin info */
typedef struct {
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    plugin_place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    char* name;
    void* handle;
    char* so_path;
} plugin_handle_t;

/* Print usage information */
void print_usage(void) {
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n"
           "Arguments:\n"
           "  queue_size   Maximum number of items in each plugin's queue\n"
           "  plugin1..N   Names of plugins to load (without .so extension)\n"
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

/* Copy a file from src to dest */
int copy_file(const char* src, const char* dest) {
    int src_fd, dest_fd;
    ssize_t n;
    char buf[4096];
    int ret = -1;

    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("open source");
        return -1;
    }

    struct stat st;
    if (fstat(src_fd, &st) == -1) {
        perror("fstat");
        close(src_fd);
        return -1;
    }

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
    if (dest_fd == -1) {
        perror("open dest");
        close(src_fd);
        return -1;
    }

    while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dest_fd, buf, n) != n) {
            perror("write");
            goto cleanup;
        }
    }

    if (n == 0) {
        ret = 0;
    } else {
        perror("read");
    }

cleanup:
    close(src_fd);
    close(dest_fd);
    if (ret != 0) {
        unlink(dest);
    }
    return ret;
}

/* Clean up all plugins */
void cleanup_plugins(plugin_handle_t* plugins, int count, char** names) {
    for (int i = 0; i < count; i++) {
        if (plugins[i].handle) {
            dlclose(plugins[i].handle);
        }
        if (plugins[i].so_path) {
            char orig[256];
            snprintf(orig, sizeof(orig), "output/%s.so", names[i]);
            if (strcmp(orig, plugins[i].so_path) != 0) {
                unlink(plugins[i].so_path);
            }
            free(plugins[i].so_path);
        }
        free(plugins[i].name);
    }
    free(plugins);
}

int main(int argc, char* argv[]) {
    
    /* Parse command-line arguments */
    if (argc < 3) {
        fprintf(stderr, "Error: Missing arguments.\n");
        print_usage();
        fflush(stdout);
        exit(1);
    }
    
    int queue_size = atoi(argv[1]);
    if (queue_size <= 0) {
        fprintf(stderr, "Error: queue_size must be a positive integer.\n");
        print_usage();
        fflush(stdout);
        exit(1);
    }
    
    int num_plugins = argc - 2;
    char** plugin_names = &argv[2];
    
    plugin_handle_t* plugins = calloc(num_plugins, sizeof(plugin_handle_t));
    if (!plugins) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        exit(1);
    }
    
    /* Load all plugin shared objects */
    for (int i = 0; i < num_plugins; i++) {
        char src_path[256];
        char final_path[256];
        
        snprintf(src_path, sizeof(src_path), "output/%s.so", plugin_names[i]);

        /* Check if this plugin name was used before */
        int is_reused = 0;
        for (int j = 0; j < i; j++) {
            if (strcmp(plugin_names[j], plugin_names[i]) == 0) {
                is_reused = 1;
                break;
            }
        }

        /* If reused, make a copy with unique name */
        if (is_reused) {
            snprintf(final_path, sizeof(final_path), "output/%s.%d.so", plugin_names[i], i);
            if (copy_file(src_path, final_path) != 0) {
                fprintf(stderr, "Error: Failed to copy plugin %s\n", src_path);
                print_usage();
                fflush(stdout);
                cleanup_plugins(plugins, i, plugin_names);
                exit(1);
            }
        } else {
            strncpy(final_path, src_path, sizeof(final_path) - 1);
            final_path[sizeof(final_path) - 1] = '\0';
        }

        plugins[i].so_path = strdup(final_path);
        if (!plugins[i].so_path) {
            fprintf(stderr, "Error: strdup failed.\n");
            exit(1);
        }
        
        /* Load the plugin */
        plugins[i].handle = dlopen(final_path, RTLD_NOW | RTLD_LOCAL);
        if (!plugins[i].handle) {
            fprintf(stderr, "Error loading plugin %s: %s\n", final_path, dlerror());
            print_usage();
            fflush(stdout);
            cleanup_plugins(plugins, i + 1, plugin_names);
            exit(1);
        }
        
        /* Get function pointers */
        plugins[i].init = (plugin_init_func_t)dlsym(plugins[i].handle, "plugin_init");
        plugins[i].fini = (plugin_fini_func_t)dlsym(plugins[i].handle, "plugin_fini");
        plugins[i].place_work = (plugin_place_work_func_t)dlsym(plugins[i].handle, "plugin_place_work");
        plugins[i].attach = (plugin_attach_func_t)dlsym(plugins[i].handle, "plugin_attach");
        plugins[i].wait_finished = (plugin_wait_finished_func_t)dlsym(plugins[i].handle, "plugin_wait_finished");
        
        const char* err = dlerror();
        if (err) {
            fprintf(stderr, "Error resolving symbols in %s: %s\n", final_path, err);
            print_usage();
            fflush(stdout);
            cleanup_plugins(plugins, i + 1, plugin_names);
            exit(1);
        }
        
        plugins[i].name = strdup(plugin_names[i]);
    }
    
    /* Initialize all plugins */
    for (int i = 0; i < num_plugins; i++) {
        const char* err = plugins[i].init(queue_size);
        if (err) {
            fprintf(stderr, "Error initializing plugin %s: %s\n", plugins[i].name, err);
            cleanup_plugins(plugins, num_plugins, plugin_names);
            exit(2);
        }
    }
    
    /* Connect plugins in chain */
    for (int i = 0; i < num_plugins - 1; i++) {
        plugins[i].attach(plugins[i + 1].place_work);
    }
    
    /* Read from stdin and send to first plugin */
    char line[1026];
    int sent_end = 0;

    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0';
        const char* err = plugins[0].place_work(line);
        if (err) {
            fprintf(stderr, "Error sending work to first plugin: %s\n", err);
            cleanup_plugins(plugins, num_plugins, plugin_names);
            exit(1);
        }
        
        if (strcmp(line, "<END>") == 0) {
            sent_end = 1;
            break;
        }
    }

    /* If EOF reached before <END>, send it now */
    if (!sent_end) {
        const char* err = plugins[0].place_work("<END>");
        if (err) {
            fprintf(stderr, "Error sending <END> to first plugin: %s\n", err);
            cleanup_plugins(plugins, num_plugins, plugin_names);
            exit(1);
        }
    }
    
    /* Wait for all plugins to finish */
    for (int i = 0; i < num_plugins; i++) {
        const char* err = plugins[i].wait_finished();
        if (err) {
            fprintf(stderr, "Error waiting for plugin %s to finish: %s\n", plugins[i].name, err);
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < num_plugins; i++) {
        plugins[i].fini();
        dlclose(plugins[i].handle);
        free(plugins[i].name);
        
        /* Remove temp .so files */
        if (plugins[i].so_path) {
            char orig[256];
            snprintf(orig, sizeof(orig), "output/%s.so", plugin_names[i]);
            if (strcmp(orig, plugins[i].so_path) != 0) {
                unlink(plugins[i].so_path);
            }
            free(plugins[i].so_path);
        }
    }
    free(plugins);
    
    printf("Pipeline shutdown complete\n");
    exit(0);
}