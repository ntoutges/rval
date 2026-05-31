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

    char cbuf[100];

    // Declare local vars
    assert(rval_decl_i8(&rv, "i8", false));
    assert(rval_decl_i16(&rv, "i16", false));
    assert(rval_decl_i32(&rv, "i32", false));
    assert(rval_decl_str(&rv, "str", NULL, 10, false));
    assert(rval_decl_str(&rv, "str2", cbuf, sizeof(cbuf), false));

    // ======== Ensure setting values triggers updates ========
    assert(rval_set_i(&rv, "i8", 10));
    
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i8") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 10);

    assert(rval_set_i(&rv, "i16", 3141));
    
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i16") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 3141);

    assert(rval_set_i(&rv, "i32", 0x12345678));
    
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i32") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 0x12345678);

    assert(rval_set_str(&rv, "str", "Hello!"));

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "str") == 0);
    assert(strcmp(cmd_ogets(&sh2, 2, ""), "Hello!") == 0);

    assert(rval_set_str(&rv, "str2", "Some fairly long \"string\" that must \"be\" escaped :)"));

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "str2") == 0);
    assert(strcmp(cmd_ogets(&sh2, 2, ""), "Some fairly long \"string\" that must \"be\" escaped :)") == 0);

    // ======== Ensure duplicate sets don't trigger updates ========
    cmd_recvs(&sh2, "!\n"); // Reset shell recv state

    assert(rval_set_i(&rv, "i8", 10));
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "") == 0);

    assert(rval_set_i(&rv, "i16", 3141));
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "") == 0);

    assert(rval_set_i(&rv, "i32", 0x12345678));
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "") == 0);

    assert(rval_set_str(&rv, "str", "Hello!"));
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "") == 0);

    assert(rval_set_str(&rv, "str2", "Some fairly long \"string\" that must \"be\" escaped :)"));
    assert(strcmp(cmd_ogets(&sh2, 0, ""), "") == 0);

    // ======== Ensure 'req' is acted upon ========
    cmd_recvs(&shell, "!req i8 9\n");
    assert(rval_get_i(&rv, "i8") == 9);

    cmd_recvs(&shell, "!req i16 3140\n");
    assert(rval_get_i(&rv, "i16") == 3140);

    cmd_recvs(&shell, "!req i32 123456\n");
    assert(rval_get_i(&rv, "i32") == 123456);

    cmd_recvs(&shell, "!req str \"Testing\"\n");
    assert(strcmp(rval_get_str(&rv, "str"), "Testing") == 0);

    cmd_recvs(&shell, "!req str2 \"My grand plan\\\\is that I will be remembered\\\\My grand plan\\\\Just you wait and see!\"\n");
    assert(strcmp(rval_get_str(&rv, "str2"), "My grand plan\\is that I will be remembered\\My grand plan\\Just you wait and see!") == 0);

    // ======== Ensure 'res' is acted upon ========
    cmd_recvs(&shell, "!res i8\n");

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i8") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 9);

    cmd_recvs(&shell, "!res i16\n");

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i16") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 3140);

    cmd_recvs(&shell, "!res i32\n");

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "i32") == 0);
    assert(cmd_ogeti(&sh2, 2, 0) == 123456);

    cmd_recvs(&shell, "!res str\n");

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "str") == 0);
    assert(strcmp(cmd_ogets(&sh2, 2, ""), "Testing") == 0);

    cmd_recvs(&shell, "!res str2\n");

    assert(strcmp(cmd_ogets(&sh2, 0, ""), "set") == 0);
    assert(strcmp(cmd_ogets(&sh2, 1, ""), "str2") == 0);
    assert(strcmp(cmd_ogets(&sh2, 2, ""), "My grand plan\\is that I will be remembered\\My grand plan\\Just you wait and see!") == 0);

    // ======== Ensure 'set' is ignored ========
    cmd_recvs(&shell, "!set i8 10\n");
    assert(rval_get_i(&rv, "i8") == 9);

    cmd_recvs(&shell, "!set i16 3141\n");
    assert(rval_get_i(&rv, "i16") == 3140);

    cmd_recvs(&shell, "!set i32 123457\n");
    assert(rval_get_i(&rv, "i32") == 123456);

    cmd_recvs(&shell, "!set str \"ignoreme\"\n");
    assert(strcmp(rval_get_str(&rv, "str"), "Testing") == 0);

    cmd_recvs(&shell, "!set str2 shorty\n");
    assert(strcmp(rval_get_str(&rv, "str2"), "My grand plan\\is that I will be remembered\\My grand plan\\Just you wait and see!") == 0);

    printf("All local test cases passed successfully!\n");

    return 0;
}