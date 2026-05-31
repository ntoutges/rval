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

char lastSet[100] = { 0x00 };
char lastRes[100] = { 0x00 };

bool defRes = true;

bool onSet(rval_key_view_t key) {
    memcpy(lastSet, key.chars, key.length);
    lastSet[key.length] = 0x00;

    return true;
}

bool onRes(rval_key_view_t key) {
    memcpy(lastRes, key.chars, key.length);
    lastRes[key.length] = 0x00;

    return defRes;
}

int main() {
    shell = cmd_f('!', 10, 128, send);
    rval_f(&rv, &shell, 128, onSet, onRes);

    sh2 = cmd_f('!', 10, 128, NULL);

    // Declare local var
    assert(rval_decl_i8(&rv, "i8", false));

    // ======== Set value and esnure 'onSet' called ========
    assert(rval_set_i(&rv, "i8", 10));
    assert(strcmp(lastSet, "i8") == 0); // lastSet changed
    assert(strcmp(lastRes, "") == 0);   // lastRes unchanged

    // Reset values
    lastSet[0] = 0x00;
    lastRes[0] = 0x00;

    // ======== `Res` value and esnure 'onSet' called ========
    cmd_recvs(&shell, "!res i8\n");
    assert(strcmp(lastSet, "") == 0);   // lastSet unchanged
    assert(strcmp(lastRes, "i8") == 0); // lastRes changed

    // Ensure cmd sent
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i8") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 10);

    // Reset values
    lastSet[0] = 0x00;
    lastRes[0] = 0x00;
    cmd_recvs(&sh2, "!\n");

    // ======== `Res` value with inhibit and ensure nothing called ========
    defRes = false;                     // Inhibit!
    cmd_recvs(&shell, "!res i8\n");
    assert(strcmp(lastSet, "") == 0);   // lastSet unchanged
    assert(strcmp(lastRes, "i8") == 0); // lastRes changed

    // Ensure cmd _NOT_ sent (inhibited)
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "") == 0);

    printf("All cb test cases passed successfully!\n");

    return 0;
}