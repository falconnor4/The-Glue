#include <stdio.h>
#include "simple_header.h"

int add(int a, int b) {
    return a + b;
}

void print_message(const char* msg) {
    if (msg) {
        printf("Message: %s\n", msg);
    }
}

double average(double x, double y) {
    return (x + y) / 2.0;
}
