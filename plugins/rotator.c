/* */
#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

/**
 * Transformation function for the rotator.
 * Moves every character one position to the right. Last char wraps to front.
 */
const char* plugin_transform(const char* input) {
    size_t len = strlen(input);
    if (len == 0) {
        return strdup(input);
    }
    
    char* new_str = malloc(len + 1);
    if (!new_str) {
        return NULL;
    }
    
    /* Place the last character at the front */
    new_str[0] = input[len - 1];
    
    /*
     * Copy (len - 1) bytes from the original string.
     * This copies input[0] through input[len-2].
     */
    memcpy(new_str + 1, input, len - 1);
    
    /* Add the new null terminator at the end */
    new_str[len] = '\0';
    
    return new_str;
}

/**
 * Initialization function for the rotator plugin.
 */
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "rotator", queue_size);
}

