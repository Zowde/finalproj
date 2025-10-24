#!/bin/bash

# --- Colors for output ---
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# --- Helper functions for printing ---
print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAIL_COUNT=$((FAIL_COUNT + 1))
}

# --- Counters ---
TEST_COUNT=0
FAIL_COUNT=0

# --- Test runner function ---
# Usage: run_test "Test Name" "command_to_run" "expected_stdout" "expected_stderr"
run_test() {
    TEST_NAME="$1"
    COMMAND="$2"
    EXPECTED_STDOUT="$3"
    EXPECTED_STDERR="$4"
    
    TEST_COUNT=$((TEST_COUNT + 1))
    
    # Execute command, capturing stdout and stderr
    # Use eval to handle commands with pipes and redirects
    OUTPUT=$(eval "$COMMAND" 2> stderr.log)
    STDERR_OUTPUT=$(cat stderr.log)
    
    # Normalize expected output
    # FIX: Use printf %b instead of echo -e.
    # echo -e adds an extra trailing newline, which breaks the comparison.
    # printf %b processes escapes (like \n) without adding an extra newline.
    EXPECTED_STDOUT=$(printf "%b" "$EXPECTED_STDOUT")
    EXPECTED_STDERR=$(echo -e "$EXPECTED_STDERR")

    PASSED=true
    
    # Check STDOUT
    # We support two modes:
    # 1. Exact match (default)
    # 2. Contains match (if expected starts with "CONTAINS:")
    if [[ "$EXPECTED_STDOUT" == "CONTAINS:"* ]]; then
        # CONTAINS check
        local expected_content=${EXPECTED_STDOUT#CONTAINS:}
        if ! echo "$OUTPUT" | grep -qF "$expected_content"; then
            print_fail "$TEST_NAME (STDOUT)"
            echo "  Expected (to contain): $expected_content"
            echo "  Got: $OUTPUT"
            PASSED=false
        fi
    else
        # EXACT match check
        if [ "$OUTPUT" != "$EXPECTED_STDOUT" ]; then
            print_fail "$TEST_NAME (STDOUT)"
            
            # REVERT: Print output directly to terminal instead of files
            echo -e "  Expected:\n$EXPECTED_STDOUT"
            echo -e "  Got:\n$OUTPUT"
            
            PASSED=false
        fi
    fi
    
    # Check STDERR (using grep for partial match)
    if [ -n "$EXPECTED_STDERR" ]; then
        if ! echo "$STDERR_OUTPUT" | grep -qF "$EXPECTED_STDERR"; then
             print_fail "$TEST_NAME (STDERR)"
             echo "  Expected (to contain): $EXPECTED_STDERR"
             echo "  Got: $STDERR_OUTPUT"
             PASSED=false
        fi
    elif [ -n "$STDERR_OUTPUT" ]; then
         print_fail "$TEST_NAME (STDERR)"
         echo "  Expected: (no stderr)"
         echo "  Got: $STDERR_OUTPUT"
         PASSED=false
    fi
    
    if $PASSED; then
        print_pass "$TEST_NAME"
    fi
    
    rm -f stderr.log
}

# --- Main Test Script ---

# 1. Run the build script first
echo "--- Running Build Script ---"
./build.sh
if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR]${NC} Build failed. Aborting tests."
    exit 1
fi
echo "--- Build Successful ---"
echo ""
echo "--- Running Tests ---"

# --- Argument Validation Tests ---
# We no longer need the giant fragile string.
# We will check that stdout "CONTAINS:" the word "Usage:"
# and stderr "contains" the correct error message.

run_test "Test 1: Arg Validation (No args)" \
         "./output/analyzer" \
         "CONTAINS:Usage:" \
         "Error: Missing arguments."

run_test "Test 2: Arg Validation (Too few)" \
         "./output/analyzer 10" \
         "CONTAINS:Usage:" \
         "Error: Missing arguments."

run_test "Test 3: Arg Validation (Invalid queue size)" \
         "./output/analyzer 0 uppercaser" \
         "CONTAINS:Usage:" \
         "Error: queue_size must be a positive integer."

run_test "Test 4: Plugin Loading (Invalid name)" \
         "echo '<END>' | ./output/analyzer 10 invalidplugin" \
         "CONTAINS:Usage:" \
         "Error loading plugin output/invalidplugin.so"

# --- Pipeline Logic Tests ---

run_test "Test 5: Single Plugin (uppercaser)" \
         "echo -e 'hello\n<END>' | ./output/analyzer 10 uppercaser" \
         "Pipeline shutdown complete" \
         ""

run_test "Test 6: Two Plugins (uppercaser -> logger)" \
         "echo -e 'hello\n<END>' | ./output/analyzer 10 uppercaser logger" \
         "[logger] HELLO\nPipeline shutdown complete" \
         ""

run_test "Test 7: Full Pipeline (PDF Example)" \
         "echo -e 'hello\n<END>' | ./output/analyzer 20 uppercaser rotator logger flipper typewriter" \
         "[logger] OHELL\n[typewriter] LLEHO\nPipeline shutdown complete" \
         ""

run_test "Test 8: Rotator Logic (abc -> cab)" \
         "echo -e 'abc\n<END>' | ./output/analyzer 10 rotator logger" \
         "[logger] cab\nPipeline shutdown complete" \
         ""

run_test "Test 9: Flipper Logic (world -> dlrow)" \
         "echo -e 'world\n<END>' | ./output/analyzer 10 flipper logger" \
         "[logger] dlrow\nPipeline shutdown complete" \
         ""

run_test "Test 10: Expander Logic (hi -> h i)" \
         "echo -e 'hi\n<END>' | ./output/analyzer 10 expander logger" \
         "[logger] h i\nPipeline shutdown complete" \
         ""

# --- Edge Case Tests ---

run_test "Test 11: Empty String Input" \
         "echo -e '\n<END>' | ./output/analyzer 10 uppercaser rotator flipper expander logger" \
         "[logger] \nPipeline shutdown complete" \
         ""

run_test "Test 12: Multiple Lines" \
         "echo -e 'one\ntwo\n<END>' | ./output/analyzer 10 uppercaser logger" \
         "[logger] ONE\n[logger] TWO\nPipeline shutdown complete" \
         ""

run_test "Test 13: Chain with Typewriter (check traffic jam)" \
         "echo -e 'fast\n<END>' | ./output/analyzer 10 typewriter logger" \
         "[typewriter] fast\n[logger] fast\nPipeline shutdown complete" \
         ""

run_test "Test 14: Re-using Plugin (rotator -> rotator)" \
         "echo -e 'abc\n<END>' | ./output/analyzer 10 rotator rotator logger" \
         "[logger] bca\nPipeline shutdown complete" \
         ""

run_test "Test 15: Concurrency (Small Queue Size)" \
         "echo -e 'one\ntwo\nthree\n<END>' | ./output/analyzer 1 uppercaser logger" \
         "[logger] ONE\n[logger] TWO\n[logger] THREE\nPipeline shutdown complete" \
         ""

run_test "Test 16: No Input (Immediate EOF)" \
         "cat /dev/null | ./output/analyzer 10 logger" \
         "Pipeline shutdown complete" \
         ""

run_test "Test 17: <END> Only" \
         "echo '<END>' | ./output/analyzer 10 logger" \
         "Pipeline shutdown complete" \
         ""

# Generate a 1024-char string (a...a)
LONG_STRING=$(printf 'a%.0s' {1..1024})
run_test "Test 18: Max Length String (1024 chars)" \
         "echo -e '${LONG_STRING}\n<END>' | ./output/analyzer 10 expander" \
         "Pipeline shutdown complete" \
         ""

# Generate a 1025-char string (b...b)
OVER_STRING=$(printf 'b%.0s' {1..1025})
# Create the expected output strings
# The line buffer is 1026, so fgets reads all 1025 chars at once.
# The 2nd read will be '\n', which main.c strips to "".
EXPECTED_B_1025=$(printf 'B%.0s' {1..1025})
run_test "Test 19: Oversized String (1025 chars)" \
         "echo -e '${OVER_STRING}\n<END>' | ./output/analyzer 10 uppercaser logger" \
         "[logger] ${EXPECTED_B_1025}\n[logger] \nPipeline shutdown complete" \
         ""

run_test "Test 20: All Plugins Chain" \
         "echo -e 'Hello World\n<END>' | ./output/analyzer 10 uppercaser expander flipper rotator logger typewriter" \
         "CONTAINS:Pipeline shutdown complete" \
         ""

# --- Summary ---
echo ""
echo "--- Test Summary ---"
if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}All $TEST_COUNT tests passed!${NC}"
    exit 0
else
    echo -e "${RED}$FAIL_COUNT / $TEST_COUNT tests failed.${NC}"
    exit 1
fi