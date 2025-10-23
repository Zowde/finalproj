#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Uppercaser Plugin
 * Converts all alphabetic characters in the string to uppercase
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
    
    for (size_t i = 0; i < len; i++) {
        output[i] = (char)toupper((unsigned char)input[i]);
    }
    output[len] = '\0';
    
    return output;
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "uppercaser", queue_size);
}