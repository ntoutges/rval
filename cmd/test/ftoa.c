#include "../cmd_str.h"
#include <stdio.h>
#include <assert.h>

int main() {
    char buf[100];

    // Check basic ints
    for (int i = -99; i < 256; i++) {
        assert(cmd_str_atoi(cmd_str_ftoma(i, buf, 5)) == i);
    }

    // // Basic checks
    assert(strcmp(cmd_str_ftoma(1.234, buf, sizeof(buf)), "1.234") == 0);
    assert(strcmp(cmd_str_ftoma(3.1415926, buf, 10), "3.1415926") == 0);

    // Check rounding
    assert(strcmp(cmd_str_ftoma(1.99999999, buf, 5), "2") == 0);
    assert(strcmp(cmd_str_ftoma(9.99999999, buf, 5), "10") == 0);
    assert(strcmp(cmd_str_ftoma(9999, buf, 5), "9999") == 0);
    assert(strcmp(cmd_str_ftoma(99999, buf, 5), "10E4") == 0);
    assert(strcmp(cmd_str_ftoma(9.99999999999e98, buf, 5), "1E99") == 0);
    assert(strcmp(cmd_str_ftoma(9.99999999999e99, buf, 5), "Inf") == 0); // Need to increase size of exponent, but cannot; INF
    
    // Compression Tests
    assert(strcmp(cmd_str_ftoma(1.234e0, buf, 4), "1.2") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e1, buf, 4), "12") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e2, buf, 4), "123") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e3, buf, 4), "1E3") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e4, buf, 4), "1E4") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e5, buf, 4), "1E5") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e6, buf, 4), "1E6") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e7, buf, 4), "1E7") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e8, buf, 4), "1E8") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e9, buf, 4), "1E9") == 0);
    assert(strcmp(cmd_str_ftoma(1.234e10, buf, 4), "Inf") == 0);
    assert(strcmp(cmd_str_ftoma(-1.234e0, buf, 4), "+-1") == 0);
    assert(strcmp(cmd_str_ftoma(-1.234e1, buf, 4), "+-I") == 0);
    assert(strcmp(cmd_str_ftoma(-1.234e2, buf, 4), "+-I") == 0);

    // Infinity Compression Tests
    assert(strcmp(cmd_str_ftoma(INFINITY, buf, sizeof(buf)), "Infinity") == 0);
    assert(strcmp(cmd_str_ftoma(INFINITY, buf, 9), "Infinity") == 0);
    assert(strcmp(cmd_str_ftoma(INFINITY, buf, 8), "Inf") == 0);
    assert(strcmp(cmd_str_ftoma(INFINITY, buf, 4), "Inf") == 0);
    assert(strcmp(cmd_str_ftoma(INFINITY, buf, 3), "I") == 0);
    assert(strcmp(cmd_str_ftoma(INFINITY, buf, 2), "I") == 0);
    assert(strcmp(cmd_str_ftoma(-INFINITY, buf, sizeof(buf)), "+-Infinity") == 0);
    assert(strcmp(cmd_str_ftoma(-INFINITY, buf, 11), "+-Infinity") == 0);
    assert(strcmp(cmd_str_ftoma(-INFINITY, buf, 10), "+-Inf") == 0);
    assert(strcmp(cmd_str_ftoma(-INFINITY, buf, 6), "+-Inf") == 0);
    assert(strcmp(cmd_str_ftoma(-INFINITY, buf, 5), "+-I") == 0);
    assert(strcmp(cmd_str_ftoma(-INFINITY, buf, 4), "+-I") == 0);
    assert(strcmp(cmd_str_ftoma(-INFINITY, buf, 3), "+") == 0);
    assert(strcmp(cmd_str_ftoma(-INFINITY, buf, 2), "+") == 0);

    // NaN Compression Tests
    assert(strcmp(cmd_str_ftoma(NAN, buf, sizeof(buf)), "NaN") == 0);
    assert(strcmp(cmd_str_ftoma(NAN, buf, 4), "NaN") == 0);
    assert(strcmp(cmd_str_ftoma(NAN, buf, 3), "N") == 0);
    assert(strcmp(cmd_str_ftoma(NAN, buf, 2), "N") == 0);
    assert(strcmp(cmd_str_ftoma(-NAN, buf, sizeof(buf)), "NaN") == 0);
    assert(strcmp(cmd_str_ftoma(-NAN, buf, 4), "NaN") == 0);
    assert(strcmp(cmd_str_ftoma(-NAN, buf, 3), "N") == 0);
    assert(strcmp(cmd_str_ftoma(-NAN, buf, 2), "N") == 0);

    printf("All ftoa test cases passed successfully!\n");

    return 0;
}