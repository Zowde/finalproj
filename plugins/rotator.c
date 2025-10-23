#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>

/**
 * Rotator Plugin
 * Moves every character in the string one position to the right
 * The last character wraps around to the front
 * Example: "hello" -> "ohell"
 */

const char* plugin_transform(const char* input) {
    if (input == NULL) {
        return NULL;
    }
    
    size_t len = strlen(input);
    
    if (len <= 1) {
        char* output = (char*)malloc(len + 1);
        if (output == NULL) {
            return NULL;
        }
        strcpy(output, input);
        return output;
    }
    
    char* output = (char*)malloc(len + 1);
    if (output == NULL) {
        return NULL;
    }
    
    /* Last character goes to front */
    output[0] = input[len - 1];
    /* All others shift right */
    for (size_t i = 1; i < len; i++) {
        output[i] = input[i - 1];
    }
    output[len] = '\0';
    
    return output;
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "rotator", queue_size);
}