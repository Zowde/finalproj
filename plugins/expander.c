/* */
#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

/**
 * Transformation function for the expander.
 * Inserts a single white space between each character.
 */
const char* plugin_transform(const char* input) {
    size_t len = strlen(input);
    if (len == 0) {
        return strdup(input);
    }
    
    /* New length will be len + (len - 1) for spaces + 1 for null */
    size_t new_len = len * 2 - 1;
    char* new_str = malloc(new_len + 1);
    if (!new_str) {
        return NULL;
    }
    
    for (size_t i = 0, j = 0; i < len; i++) {
        new_str[j++] = input[i];
        if (i < len - 1) {
            new_str[j++] = ' '; /* */
        }
    }
    new_str[new_len] = '\0';
    
    return new_str;
}

/**
 * Initialization function for the expander plugin.
 */
const char* plugin_init(int queue_size) { /* */
    return common_plugin_init(plugin_transform, "expander", queue_size); /* */
}