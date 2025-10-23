#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>

/**
 * Expander Plugin
 * Inserts a single white space between each character in the string
 * Example: "hello" -> "h e l l o"
 */

const char* plugin_transform(const char* input) {
    if (input == NULL) {
        return NULL;
    }
    
    size_t len = strlen(input);
    
    if (len == 0) {
        char* output = (char*)malloc(1);
        if (output == NULL) {
            return NULL;
        }
        output[0] = '\0';
        return output;
    }
    
    /* Output length: chars + (chars-1) spaces */
    size_t output_len = len + (len - 1);
    char* output = (char*)malloc(output_len + 1);
    if (output == NULL) {
        return NULL;
    }
    
    /* Insert spaces between characters */
    size_t out_idx = 0;
    for (size_t i = 0; i < len; i++) {
        output[out_idx++] = input[i];
        if (i < len - 1) {
            output[out_idx++] = ' ';
        }
    }
    output[out_idx] = '\0';
    
    return output;
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "expander", queue_size);
}