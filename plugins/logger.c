/* */
#include "plugin_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Transformation function for the logger.
 * Logs all strings that pass through to standard output.
 */
const char* plugin_transform(const char* input) {
    /* STDOUT must only contain pipeline printouts */
    printf("[logger] %s\n", input); /* */
    
    /* Must return a new, allocated string for the common infrastructure */
    return strdup(input);
}

/**
 * Initialization function for the logger plugin.
 * Calls the common init function.
 */
const char* plugin_init(int queue_size) { /* */
    return common_plugin_init(plugin_transform, "logger", queue_size); /* */
}