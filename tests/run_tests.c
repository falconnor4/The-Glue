#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/canonical_abi.h"

// Forward declare the wrapper functions from the manually generated shim file.
CanonicalResult canonical_wrapper_add(const char* args_buffer);
CanonicalResult canonical_wrapper_print_message(const char* args_buffer);
CanonicalResult canonical_wrapper_average(const char* args_buffer);

int total_tests = 0;
int failed_tests = 0;

void test_add() {
    total_tests++;
    printf("--- Test: add(5, 10) ---\n");

    // 1. Prepare the canonical argument buffer
    char args_buffer[sizeof(int) + sizeof(int)];
    int a = 5;
    int b = 10;
    memcpy(args_buffer, &a, sizeof(int));
    memcpy(args_buffer + sizeof(int), &b, sizeof(int));

    // 2. Call the canonical wrapper
    CanonicalResult result = canonical_wrapper_add(args_buffer);

    // 3. Check the result
    if (result.long_val == 15) {
        printf("OK: Result was %ld\n", result.long_val);
    } else {
        printf("FAIL: Expected 15, got %ld\n", result.long_val);
        failed_tests++;
    }
}

void test_print_message() {
    total_tests++;
    printf("--- Test: print_message(\"Hello Canonical ABI\") ---\n");

    // 1. Prepare buffer
    char args_buffer[sizeof(const char*)];
    const char* msg = "Hello Canonical ABI";
    memcpy(args_buffer, &msg, sizeof(const char*));

    // 2. Call wrapper
    // The function is void, so we just check that it runs without crashing.
    canonical_wrapper_print_message(args_buffer);

    printf("OK: print_message was called.\n");
}

void test_average() {
    total_tests++;
    printf("--- Test: average(3.0, 7.0) ---\n");

    // 1. Prepare buffer
    char args_buffer[sizeof(double) + sizeof(double)];
    double a = 3.0;
    double b = 7.0;
    memcpy(args_buffer, &a, sizeof(double));
    memcpy(args_buffer + sizeof(double), &b, sizeof(double));

    // 2. Call wrapper
    CanonicalResult result = canonical_wrapper_average(args_buffer);

    // 3. Check result (allowing for floating point inaccuracy)
    double expected = 5.0;
    double diff = result.double_val - expected;
    if (diff < 0) diff = -diff;

    if (diff < 1e-9) {
        printf("OK: Result was %f\n", result.double_val);
    } else {
        printf("FAIL: Expected %f, got %f\n", expected, result.double_val);
        failed_tests++;
    }
}


int main() {
    printf("=== Running Integration Tests ===\n");

    test_add();
    test_print_message();
    test_average();

    printf("=================================\n");
    if (failed_tests == 0) {
        printf("All %d tests passed!\n", total_tests);
        return 0;
    } else {
        printf("%d out of %d tests failed.\n", failed_tests, total_tests);
        return 1;
    }
}
