#include "../cmd_str.h"
#include <stdio.h>
#include <assert.h>

int main() {
    char buf[100];

    // Check basic ints
    for (int i = -1000; i < 1000; i++) {
        assert(cmd_str_atof((cmd_str_ftoma(i, buf, sizeof(buf)))) == i);
    }

    // Ensure negative values are preceeded by a '+' to prevent confusion with flags
    assert(cmd_str_ftoma(-1, buf, sizeof(buf))[0] == '+');

    // ZERO
    assert(cmd_str_atof("") == 0);
    assert(cmd_str_atof(NULL) == 0);

    // Infinity Tests
    assert(cmd_str_atof("Infinity") == INFINITY);
    assert(cmd_str_atof("Inf") == INFINITY);
    assert(cmd_str_atof("I") == INFINITY);
    assert(cmd_str_atof("Inot") == 0);
    assert(cmd_str_atof("-Infinity") == -INFINITY);
    assert(cmd_str_atof("-Inf") == -INFINITY);
    assert(cmd_str_atof("-I") == -INFINITY);
    assert(cmd_str_atof("-Inot") == 0);

    // NaN Tests
    assert(isnan(cmd_str_atof("NaN")));
    assert(isnan(cmd_str_atof("N")));
    assert(!isnan(cmd_str_atof("Nnot")));
    assert(isnan(cmd_str_atof("-NaN")));
    assert(isnan(cmd_str_atof("-N")));
    assert(!isnan(cmd_str_atof("-Nnot")));

    printf("All atof test cases passed successfully!\n");

    return 0;
}