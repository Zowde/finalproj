#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Typewriter Plugin
 * Simulates a typewriter effect by printing each character with a 100ms delay
 * Note: This can cause a "traffic jam" in the pipeline as processing is slower
 */

const char* plugin_transform(const char* input) {
    if (input == NULL) {
        return NULL;
    }
    
    size_t len = strlen(input);
    
    /* Print each character with 100ms delay */
    for (size_t i = 0; i < len; i++) {
        printf("%c", input[i]);
        fflush(stdout);
        usleep(100000);  /* 100ms = 100000 microseconds */
    }
    printf("\n");
    fflush(stdout);
    
    /* Return copy for next plugin */
    char* output = (char*)malloc(len + 1);
    if (output == NULL) {
        return NULL;
    }
    strcpy(output, input);
    
    return output;
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "typewriter", queue_size);
}