#include <stdio.h>
#include <assert.h>
#include "../cmd.h"

char cbuf[100];
int ci = 0;

void send(char ch) {
    if (ch == '\n') {
        cbuf[ci] = 0x00;
        ci = 0;
        return;
    }

    cbuf[ci++] = ch;
}

int main() {
    cmd_t sh = cmd_f('!', 10, 64, send);

    // ======== Ensure escape characters properly registered _and_ ignored ========
    assert(cmd_recvs(&sh, "!escape \"Some \\\"escaped\\\" string\" here\n"));

    assert(strcmp(cmd_ogets(&sh, 0, ""), "escape") == 0);
    assert(strcmp(cmd_ogets(&sh, 1, ""), "Some \"escaped\" string") == 0);
    assert(strcmp(cmd_ogets(&sh, 2, ""), "here") == 0);

    // ======== Test basic basic escape sequences ========
    assert(cmd_recvs(&sh, "!escaped \\\\ \\\"\n"));

    assert(strcmp(cmd_ogets(&sh, 0, ""), "escaped") == 0);
    assert(strcmp(cmd_ogets(&sh, 1, ""), "\\") == 0);
    assert(strcmp(cmd_ogets(&sh, 2, ""), "\"") == 0);

    // ======== Test Raw sender ========
    assert(cmd_sendr(&sh, "!raw escape \"not \\ escaped\"\n"));
    assert(strcmp(cbuf, "!raw escape \\\"not \\\\ escaped\\\"") == 0);

    printf("All esc test cases passed successfully!\n");

    return 0;
}