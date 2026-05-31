#include <stdio.h>
#include <assert.h>
#include "../rval.h"

#define EPSILON 1e-5

cmd_t shell;
cmd_t sh2;
rval_t rv;

char rxBuf[100];
int rxBufI = 0;

// Forward to other shell
void send(char ch) {
    cmd_recv(&sh2, ch);
}

int main() {
    shell = cmd_f('!', 10, 128, send);
    rval_f(&rv, &shell, 128, NULL, NULL);

    sh2 = cmd_f('!', 10, 128, NULL);

    // Declare remote vars
    assert(rval_decl_i8(&rv, "i8", true));
    assert(rval_decl_i16(&rv, "i16", true));
    assert(rval_decl_i32(&rv, "i32", true));

    // Attempt to change vars
    assert(rval_set_i(&rv, "i8", 123));
    assert(rval_get_i(&rv, "i8") == 0); // Still at default because remote

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "req") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i8") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 123);

    assert(rval_set_i(&rv, "i16", 0x7EDC));
    assert(rval_get_i(&rv, "i16") == 0); // Still at default because remote

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "req") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i16") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 0x7EDC);

    assert(rval_set_i(&rv, "i32", 0x12345678));
    assert(rval_get_i(&rv, "i32") == 0); // Still at default because remote

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "req") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i32") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 0x12345678);

    // Ignore other requests
    cmd_sends(&shell, "!req i8 10\n");
    assert(rval_get_i(&rv, "i8") == 0); // Still at default because remote
    cmd_sends(&shell, "!req i16 10\n");
    assert(rval_get_i(&rv, "i16") == 0); // Still at default because remote
    cmd_sends(&shell, "!req i32 10\n");
    assert(rval_get_i(&rv, "i32") == 0); // Still at default because remote

    // Update on sets
    cmd_recvs(&shell, "!set i8 81\n");
    assert(rval_get_i(&rv, "i8") == 81);
    cmd_recvs(&shell, "!set i16 168\n");
    assert(rval_get_i(&rv, "i16") == 168);
    cmd_recvs(&shell, "!set i32 328\n");
    assert(rval_get_i(&rv, "i32") == 328);

    // Don't respond to 'res' (defer to local source-of-truth)
    cmd_recvs(&sh2, "!\n"); // Clear sh2 cache
    cmd_recvs(&shell, "!res i8\n");                  // Attempt to resolve vlaue
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "") == 0); // Didn't receive response transmission (good for remote value)

    printf("All remote test cases passed successfully!\n");

    return 0;
}