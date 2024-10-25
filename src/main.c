#include <stdio.h>

#define TAM_UTF8_IMPLEMENTATION
#include "utf8.h"
#include "test.c"

int main() {
    int num_failed = run_tests();
    if (num_failed == 0) {
        printf("All tests passed!\n");
    } else {
        printf("%d tests failed\n", num_failed);
    }
}
