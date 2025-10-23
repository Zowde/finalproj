/* */
#include "plugin_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> /* For usleep */

/**
 * Transformation function for the typewriter.
 * Simulates a typewriter effect with a 100ms delay per character.
 */
const char* plugin_transform(const char* input) {
    /* Format: [typewriter] LLEHO */
    printf("[typewriter] ");
    fflush(stdout); /* Ensure prefix prints before delay */
    
    for (size_t i = 0; input[i]; i++) {
        putchar(input[i]);
        fflush(stdout);
        usleep(100000); /* 100ms delay */
    }
    printf("\n");
    fflush(stdout);
    
    return strdup(input);
}

/**
 * Initialization function for the typewriter plugin.
 */
const char* plugin_init(int queue_size) { /* */
    return common_plugin_init(plugin_transform, "typewriter", queue_size); /* */
}