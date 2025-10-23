#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Colors for output ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# --- Helper functions for printing ---
print_status() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# --- Create output directory ---
print_status "Creating output directory..."
rm -rf output
mkdir -p output

# --- Build Main Application ---
print_status "Building main application: analyzer"
# Use gcc-13 as specified in the PDF, and link against libdl (-ldl)
gcc-13 -Wall -Werror -o output/analyzer main.c -ldl -pthread || {
    print_error "Failed to build main application"
    exit 1
}

# --- Define common source files for all plugins ---
COMMON_SOURCES="plugins/plugin_common.c plugins/sync/monitor.c plugins/sync/consumer_producer.c"

# --- Build Plugins ---
PLUGINS="logger typewriter uppercaser rotator flipper expander"

for plugin_name in $PLUGINS; do
    print_status "Building plugin: $plugin_name"
    
    # Compile the plugin as a shared object (-fPIC -shared)
    # Link against libdl and libpthread
    gcc-13 -Wall -Werror -fPIC -shared \
        -o output/${plugin_name}.so \
        plugins/${plugin_name}.c \
        $COMMON_SOURCES \
        -ldl -pthread || {
        
        print_error "Failed to build $plugin_name"
        exit 1
    }
done

print_status "Build complete. All binaries are in the 'output/' directory."
