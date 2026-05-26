#include <stdio.h>
#include <assert.h>
#include "../rval.h"

char txBuf[100];
int txBufI = 0;
void tx(char ch) {
    txBuf[txBufI++] = ch;
    txBuf[txBufI] = 0x00;

    if (ch == '\n') {
        printf(" << %s", txBuf);
        txBufI = 0;
    }
}

int main() {
    cmd_t shell;
    rval_t myRval;

    shell = cmd_f('!', 10, 128, tx);
    rval_f(&myRval, &shell, 128, NULL, NULL);

    memset(myRval.buf.buf, 0, myRval.buf.cap);

    bool s1 = rval_decl_i8(&myRval, "myVar", false);

    printf("Set 0x01020304\n");
    bool s2 = rval_set_i(&myRval, "myVar", 0x01020304);

    printf("Set 0x01020305\n");
    bool s3 = rval_set_i(&myRval, "myVar", 0x01020305);

    printf(" >> !req myVar 12\n");
    cmd_recvs(&shell, "!req myVar 12\n");

    printf("Success: %d %d %d (0x%08x)\n", s1, s2, s3, rval_get_i(&myRval, "myVar"));

    // Print contents of vbuf
    printf("[ ");
    for (int i = 0; i < myRval.buf.cap; i++) {
         uint8_t ch = myRval.buf.buf[i];

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '+' || ch == '-')
            printf("%c ", ch); // Printable character
        else if (ch == 0x00)
            printf("\\ ");       // Null byte
        else
            printf("%02X ", ch); // Print out as raw HEX       
    }
    printf("]\n");

    // assert(rval_get_i(&myRval, "myVar") == 100);

    return 0;
}
