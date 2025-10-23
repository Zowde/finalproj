/* */
#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

/**
 * Transformation function for the flipper.
 * Reverses the order of characters in the string.
 */
const char* plugin_transform(const char* input) {
    size_t len = strlen(input);
    char* new_str = malloc(len + 1);
    if (!new_str) {
        return NULL;
    }
    
    for (size_t i = 0; i < len; i++) {
        new_str[i] = input[len - 1 - i]; /* */
    }
    new_str[len] = '\0';
    
    return new_str;
}

/**
 * Initialization function for the flipper plugin.
 */
const char* plugin_init(int queue_size) { /* */
    return common_plugin_init(plugin_transform, "flipper", queue_size); /* */
}