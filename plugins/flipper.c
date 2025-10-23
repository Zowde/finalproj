#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>

/**
 * Flipper Plugin
 * Reverses the order of characters in the string
 * Example: "hello" -> "olleh"
 */

const char* plugin_transform(const char* input) {
    if (input == NULL) {
        return NULL;
    }
    
    size_t len = strlen(input);
    
    char* output = (char*)malloc(len + 1);
    if (output == NULL) {
        return NULL;
    }
    
    /* Reverse the string */
    for (size_t i = 0; i < len; i++) {
        output[i] = input[len - 1 - i];
    }
    output[len] = '\0';
    
    return output;
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "flipper", queue_size);
}