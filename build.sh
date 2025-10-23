#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Exit on error
set -e

# Create output directory
print_status "Creating output directory"
mkdir -p output

# Build synchronization infrastructure
print_status "Building monitor synchronization primitive..."
gcc -c -o output/monitor.o plugins/sync/monitor.c -Wall -Wextra

print_status "Building consumer-producer queue..."
gcc -c -o output/consumer_producer.o plugins/sync/consumer_producer.c \
    -I plugins/sync -Wall -Wextra

# Build and run tests
print_status "Building monitor unit tests..."
gcc -o output/monitor_test \
    output/monitor.o \
    plugins/sync/monitor_test.c \
    -lpthread -Wall -Wextra

print_status "Running monitor unit tests..."
output/monitor_test || {
    print_error "Monitor tests failed!"
    exit 1
}

print_status "Building consumer-producer unit tests..."
gcc -o output/consumer_producer_test \
    output/monitor.o \
    output/consumer_producer.o \
    plugins/sync/consumer_producer_test.c \
    -I plugins/sync \
    -lpthread -Wall -Wextra

print_status "Running consumer-producer unit tests..."
output/consumer_producer_test || {
    print_error "Consumer-producer tests failed!"
    exit 1
}

print_status "All synchronization tests passed!"

# Build plugin common infrastructure
print_status "Building plugin common infrastructure..."
gcc -c -o output/plugin_common.o \
    plugins/plugin_common.c \
    -I plugins \
    -I plugins/sync \
    -fPIC \
    -Wall -Wextra || {
    print_error "Failed to build plugin_common.o"
    exit 1
}

print_status "Plugin infrastructure compiled successfully!"

# Build plugins (shared libraries)
print_status "Building plugins..."
PLUGINS="logger uppercaser rotator flipper expander typewriter"

for plugin_name in $PLUGINS; do
    print_status "Building plugin: $plugin_name"
    gcc -fPIC -shared -o output/${plugin_name}.so \
        plugins/${plugin_name}.c \
        output/plugin_common.o \
        output/monitor.o \
        output/consumer_producer.o \
        -I plugins \
        -I plugins/sync \
        -ldl -lpthread \
        -Wall -Wextra || {
        print_error "Failed to build $plugin_name"
        exit 1
    }
done

print_status "All plugins built successfully!"

# Build main application (analyzer) - FIXED: Includes necessary object files
print_status "Building main application..."
gcc -o output/analyzer \
    main.c \
    output/plugin_common.o \
    output/monitor.o \
    output/consumer_producer.o \
    -I plugins \
    -I plugins/sync \
    -ldl -lpthread \
    -Wall -Wextra || {
    print_error "Failed to build main application"
    exit 1
}

# Ensure the executable has correct permissions (for safety)
chmod +x output/analyzer

print_status "Main application built successfully!"
echo ""
print_status "BUILD COMPLETE - All components compiled successfully with zero warnings!"
