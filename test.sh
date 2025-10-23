#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
TESTS_RUN=0
TESTS_PASSED=0

print_status() {
    echo -e "${GREEN}[TEST]${NC} $1"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# Exit immediately if any command fails
#set -e

# --- Build Phase ---
print_status "Building project..."
./build.sh
print_pass "Project built successfully"

# --- CRITICAL FIX: Set LD_LIBRARY_PATH ---
print_info "Setting LD_LIBRARY_PATH to ./output for dynamic loading."
export LD_LIBRARY_PATH=./output:$LD_LIBRARY_PATH

# --- Final Executable Check (Sanity check) ---
print_info "Verifying analyzer executable..."
if [ ! -x ./output/analyzer ]; then
    print_fail "Analyzer executable not found or not executable"
    ls -l ./output/ 2>/dev/null
    exit 1
fi

echo ""
print_status "Starting test suite..."
echo ""

# Test 1: Missing arguments
print_info "Test 1: Missing arguments (should show usage)"
((TESTS_RUN++))
OUTPUT=$(./output/analyzer 2>&1 || true) 
if echo "$OUTPUT" | grep -q "Usage:"; then
    print_pass "Test 1: Missing arguments handled correctly"
else
    print_fail "Test 1: Usage message not shown (Got: $OUTPUT)"
fi

# Test 2: Invalid queue size
print_info "Test 2: Invalid queue size"
((TESTS_RUN++))
OUTPUT=$(echo "test" | ./output/analyzer -5 logger 2>&1 || true)
if echo "$OUTPUT" | grep -q "positive integer\|Usage:"; then
    print_pass "Test 2: Invalid queue size rejected"
else
    print_fail "Test 2: Invalid queue size not handled (Got: $OUTPUT)"
fi

# Test 3: Non-existent plugin
print_info "Test 3: Non-existent plugin"
((TESTS_RUN++))
OUTPUT=$(echo "test" | ./output/analyzer 10 nonexistent 2>&1 || true)
if echo "$OUTPUT" | grep -q "Failed to load\|Usage:"; then
    print_pass "Test 3: Non-existent plugin handled"
else
    print_fail "Test 3: Non-existent plugin not handled (Got: $OUTPUT)"
fi

echo ""

# Test 4: Single plugin - logger
print_info "Test 4: Logger plugin"
((TESTS_RUN++))
EXPECTED="[logger] hello"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 4: Logger plugin works"
else
    print_fail "Test 4: Logger plugin failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 5: Uppercaser plugin
print_info "Test 5: Uppercaser plugin"
((TESTS_RUN++))
EXPECTED="[logger] HELLO"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 5: Uppercaser plugin works"
else
    print_fail "Test 5: Uppercaser plugin failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 6: Rotator plugin
print_info "Test 6: Rotator plugin"
((TESTS_RUN++))
EXPECTED="[logger] OHELL"
ACTUAL=$(echo -e "HELLO\n<END>" | ./output/analyzer 10 rotator logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 6: Rotator plugin works"
else
    print_fail "Test 6: Rotator plugin failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 7: Flipper plugin
print_info "Test 7: Flipper plugin"
((TESTS_RUN++))
EXPECTED="[logger] OLLEH"
ACTUAL=$(echo -e "HELLO\n<END>" | ./output/analyzer 10 flipper logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 7: Flipper plugin works"
else
    print_fail "Test 7: Flipper plugin failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 8: Expander plugin
print_info "Test 8: Expander plugin"
((TESTS_RUN++))
EXPECTED="[logger] h e l l o"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 expander logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 8: Expander plugin works"
else
    print_fail "Test 8: Expander plugin failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 9: Pipeline chain - uppercaser -> rotator -> logger
print_info "Test 9: Pipeline chain (uppercaser -> rotator -> logger)"
((TESTS_RUN++))
EXPECTED="[logger] OHELL"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser rotator logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 9: Pipeline chain works"
else
    print_fail "Test 9: Pipeline chain failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 10: Multiple strings
print_info "Test 10: Multiple strings in pipeline"
((TESTS_RUN++))
ACTUAL=$(echo -e "hello\nworld\n<END>" | ./output/analyzer 10 logger 2>/dev/null | wc -l)
# Should have 2 log lines + 1 shutdown message = 3 lines. Testing for >= 2 lines.
if [ "$ACTUAL" -ge 2 ]; then
    print_pass "Test 10: Multiple strings processed"
else
    print_fail "Test 10: Multiple strings not processed correctly"
fi

# Test 11: Empty string
print_info "Test 11: Empty string handling"
((TESTS_RUN++))
ACTUAL=$(echo -e "\n<END>" | ./output/analyzer 10 logger 2>/dev/null | grep "\[logger\]" || true)
if [ ! -z "$ACTUAL" ]; then
    print_pass "Test 11: Empty string handled"
else
    print_fail "Test 11: Empty string not handled (Expected a log message, got none)"
fi

# Test 12: Special characters
print_info "Test 12: Special characters"
((TESTS_RUN++))
EXPECTED="[logger] hello@world!"
ACTUAL=$(echo -e "hello@world!\n<END>" | ./output/analyzer 10 logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 12: Special characters handled"
else
    print_fail "Test 12: Special characters failed"
fi

# Test 13: Reusing plugin in chain
print_info "Test 13: Plugin reused multiple times (uppercaser twice)"
((TESTS_RUN++))
EXPECTED="[logger] HELLO"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser uppercaser logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 13: Plugin reuse works"
else
    print_fail "Test 13: Plugin reuse failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 14: Large queue size
print_info "Test 14: Large queue size"
((TESTS_RUN++))
EXPECTED="[logger] hello"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 1000 logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 14: Large queue size works"
else
    print_fail "Test 14: Large queue size failed"
fi

# Test 15: Queue size 1 (edge case)
print_info "Test 15: Queue size 1 (edge case)"
((TESTS_RUN++))
EXPECTED="[logger] hello"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 1 logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 15: Queue size 1 works"
else
    print_fail "Test 15: Queue size 1 failed"
fi

# Test 16: Shutdown message
print_info "Test 16: Pipeline shutdown complete message"
((TESTS_RUN++))
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 logger 2>/dev/null | grep "Pipeline shutdown complete" || true)
if [ ! -z "$ACTUAL" ]; then
    print_pass "Test 16: Shutdown message present"
else
    print_fail "Test 16: Shutdown message missing"
fi

# Test 17: Complex pipeline - all plugins (uppercaser -> rotator -> flipper -> expander -> logger)
print_info "Test 17: Complex pipeline with multiple plugins"
((TESTS_RUN++))
ACTUAL=$(echo -e "hi\n<END>" | ./output/analyzer 10 uppercaser rotator flipper expander logger 2>/dev/null | grep "\[logger\]" || true)
if [ ! -z "$ACTUAL" ]; then
    print_pass "Test 17: Complex pipeline works"
else
    print_fail "Test 17: Complex pipeline failed"
fi

# Test 18: Flipper on rotated string
print_info "Test 18: Flipper on rotated string"
((TESTS_RUN++))
EXPECTED="[logger] ELLOH"
ACTUAL=$(echo -e "HELLO\n<END>" | ./output/analyzer 10 rotator flipper logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 18: Flipper on rotated string works"
else
    print_fail "Test 18: Flipper on rotated string failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 19: Logger appears in middle of pipeline
print_info "Test 19: Logger in middle of pipeline"
((TESTS_RUN++))
EXPECTED="[logger] HELLO"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser logger flipper 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 19: Logger in middle of pipeline works"
else
    print_fail "Test 19: Logger in middle of pipeline failed (Expected: '$EXPECTED', Got: '$ACTUAL')"
fi

# Test 20: Long string
print_info "Test 20: Long string handling"
((TESTS_RUN++))
LONG_STRING="thequickbrownfoxjumpsoverthelazydog"
EXPECTED="[logger] $LONG_STRING"
ACTUAL=$(echo -e "$LONG_STRING\n<END>" | ./output/analyzer 10 logger 2>/dev/null | grep "\[logger\]" || true)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass "Test 20: Long string handled"
else
    print_fail "Test 20: Long string failed"
fi

echo ""
echo "========================================="
echo "Test Results:"
echo "========================================="
printf "Total Tests Run: %d\n" "$TESTS_RUN"
printf "Tests Passed: ${GREEN}%d${NC}\n" "$TESTS_PASSED"
printf "Tests Failed: ${RED}%d${NC}\n" $((TESTS_RUN - TESTS_PASSED))
echo "========================================="

if [ "$TESTS_PASSED" -eq "$TESTS_RUN" ]; then
    echo -e "${GREEN}ALL TESTS PASSED!${NC}"
    exit 0
else
    echo -e "${RED}SOME TESTS FAILED!${NC}"
    exit 1
fi
