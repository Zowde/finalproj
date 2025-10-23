#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Logger Plugin
 * Logs all strings that pass through to standard output
 * Prints: [logger] <string>
 */

const char* plugin_transform(const char* input) {
    if (input == NULL) {
        return NULL;
    }
    
    printf("[logger] %s\n", input);
    
    char* output = (char*)malloc(strlen(input) + 1);
    if (output == NULL) {
        return NULL;
    }
    strcpy(output, input);
    
    return output;
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "logger", queue_size);
}