/* */
#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * Transformation function for the uppercaser.
 * Converts all alphabetic characters in the string to uppercase.
 */
const char* plugin_transform(const char* input) {
    char* new_str = strdup(input);
    if (!new_str) {
        return NULL; /* Common infrastructure will handle this */
    }
    
    for (int i = 0; new_str[i]; i++) {
        new_str[i] = toupper(new_str[i]); /* */
    }
    
    return new_str;
}

/**
 * Initialization function for the uppercaser plugin.
 */
const char* plugin_init(int queue_size) { /* */
    return common_plugin_init(plugin_transform, "uppercaser", queue_size); /* */
}