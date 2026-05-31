#include <stdio.h>
#include <assert.h>
#include "../rval.h"

#define EPSILON 1e-5

cmd_t shell;
rval_t rv;

int main() {
    shell = cmd_f('!', 10, 128, NULL);
    rval_f(&rv, &shell, 128, NULL, NULL);

    // Ensure no values already exist
    // Test via ensurinv values evaluate to defaults
    assert(rval_get_i(&rv, "myI8") == 0);
    assert(rval_get_f(&rv, "myF") == 0);
    assert(rval_get_str(&rv, "myStr") == NULL);

    // Declare new values
    assert(rval_decl_i8(&rv, "myI8", false));
    assert(rval_decl_i16(&rv, "myI16", false));
    assert(rval_decl_i32(&rv, "myI32", false));
    assert(rval_decl_f(&rv, "myF", false));
    assert(rval_decl_str(&rv, "myStr", NULL, 10, false));

    // Ensure default values are 0/""
    assert(rval_get_i(&rv, "myI8") == 0);
    assert(rval_get_i(&rv, "myI16") == 0);
    assert(rval_get_i(&rv, "myI32") == 0);
    assert(rval_get_f(&rv, "myF") == 0);
    assert(strcmp(rval_get_str(&rv, "myStr"), "") == 0);

    // Attempt to set values
    assert(rval_set_i(&rv, "myI8", 0x76543210));    // 1 B; should only pick up 0x10
    assert(rval_set_i(&rv, "myI16", 0xA9876543));   // 2 B; should only pick up 0x6543
    assert(rval_set_i(&rv, "myI32", 0x7EDCBA98));   // 4 B; should pick up entire 0x7EDCBA98
    assert(rval_set_f(&rv, "myF", 3.1415926));
    assert(rval_set_str(&rv, "myStr", "cstrings!"));

    // Ensure values are set
    assert(rval_get_i(&rv, "myI8") == 0x10);
    assert(rval_get_i(&rv, "myI16") == 0x6543);
    assert(rval_get_i(&rv, "myI32") == 0x7EDCBA98);
    assert(fabs(rval_get_f(&rv, "myF") - 3.1415926) < EPSILON);
    assert(strcmp(rval_get_str(&rv, "myStr"), "cstrings!") == 0);

    // Ensure invalid strings cannot be set
    assert(!rval_set_str(&rv, "myStr", "too long!!"));
    assert(strcmp(rval_get_str(&rv, "myStr"), "cstrings!") == 0); // Ensure value _didn't_ change

    // Ensure type checks
    assert(!rval_set_f(&rv, "myI8", 123));   // Set i8 with f
    assert(rval_get_i(&rv, "myI8") == 0x10); // Unchanged
    assert(!rval_set_f(&rv, "myI16", 123));  // Set i16 with f
    assert(rval_get_i(&rv, "myI16") == 0x6543); // Unchanged
    assert(!rval_set_f(&rv, "myI32", 123));     // Set i32 with f
    assert(rval_get_i(&rv, "myI32") == 0x7EDCBA98); // Unchanged
    assert(!rval_set_i(&rv, "myF", 123));           // Set f with i
    assert(fabs(rval_get_f(&rv, "myF") - 3.1415926) < EPSILON); // Unchanged
    assert(!rval_set_i(&rv, "myStr", 123));                        // Set str with i
    assert(strcmp(rval_get_str(&rv, "myStr"), "cstrings!") == 0); // Unchanged

    // Test variable name bounds
    assert(!rval_decl_i8(&rv, "", false)); // 0-length key: REJECT
    assert(rval_decl_i8(&rv, "a", false)); // 1-length key: ACCEPT
    assert(rval_decl_i8(&rv, "testofthelongkeythatIwillaccept", false)); // 31-length key: ACCEPT
    assert(!rval_decl_i8(&rv, "testofthelongkeythatIwillreject!", false)); // 32-length key: REJECT

    // Test string pointing to external buffer
    char external[10];
    assert(rval_decl_str(&rv, "ext", external, sizeof(external), false));
    assert(strcmp(rval_get_str(&rv, "ext"), "") == 0); // Default value (empty str)
    assert(rval_set_str(&rv, "ext", "my 9 char"));              // Set value
    assert(strcmp(rval_get_str(&rv, "ext"), "my 9 char") == 0); // Check
    assert(!rval_set_str(&rv, "ext", "my 9 chars"));            // Too long
    assert(strcmp(rval_get_str(&rv, "ext"), "my 9 char") == 0); // Check for no diff

    // Ensure variables don't overrun bounds/overwrite cache
    // Fill buffer
    char maxC = 'A';
    while (maxC <= 'Z') {
        char cBuf[2] = { maxC, 0x00 };

        if (!rval_decl_i8(&rv, cBuf, false)) break;
        rval_set_i(&rv, cBuf, maxC + 0x20); // Make value lowercase version of key
        maxC++;
    }

    // Check variable values
    for (char i = 'A'; i < maxC; i++) {
        char cBuf[2] = { i, 0x00 };
        assert(rval_get_i(&rv, cBuf) == i + 0x20);
    }

    // Ensure values are (still) set
    assert(rval_get_i(&rv, "myI8") == 0x10);
    assert(rval_get_i(&rv, "myI16") == 0x6543);
    assert(rval_get_i(&rv, "myI32") == 0x7EDCBA98);
    assert(fabs(rval_get_f(&rv, "myF") - 3.1415926) < EPSILON);
    assert(strcmp(rval_get_str(&rv, "myStr"), "cstrings!") == 0);
    assert(rval_get_i(&rv, "a") == 0); // Default
    assert(rval_get_i(&rv, "testofthelongkeythatIwillaccept") == 0); // Default

    // Undeclare values
    assert(rval_undecl(&rv, "myI8"));
    assert(rval_get_i(&rv, "myI8") == 0); // Should no longer exist
    assert(rval_undecl(&rv, "myI16"));
    assert(rval_get_i(&rv, "myI16") == 0); // Should no longer exist
    assert(rval_undecl(&rv, "myI32"));
    assert(rval_get_i(&rv, "myI32") == 0); // Should no longer exist
    assert(rval_undecl(&rv, "myF"));
    assert(rval_get_f(&rv, "myI32") == 0); // Should no longer exist
    assert(rval_undecl(&rv, "myStr"));
    assert(rval_get_str(&rv, "myStr") == NULL); // Should no longer exist
    assert(rval_undecl(&rv, "a"));
    assert(rval_get_i(&rv, "a") == 0); // Should no longer exist
    assert(rval_undecl(&rv, "testofthelongkeythatIwillaccept"));
    assert(rval_get_i(&rv, "testofthelongkeythatIwillaccept") == 0); // Should no longer exist
    assert(rval_undecl(&rv, "ext"));
    assert(rval_get_str(&rv, "ext") == NULL); // Should no longer exist

    // Undeclare fillers
    for (char i = 'A'; i < maxC; i++) {
        char cBuf[2] = { i, 0x00 };
        assert(rval_undecl(&rv, cBuf));
    }

    // Allocate maximum number of values for 128-size buffer
    // Ensures everything properly cleaned up
    const int maxI32 = 18;
    for (int i = 0; i < maxI32; i++) { // floor(128 / (1 (header) + 1 (key) + 4 (data) + 1 (cache)))
        char buf[] = { 'a' + i, 0x00 };
        assert(rval_decl_i32(&rv, buf, false));
        
        // Set to some value
        assert(rval_set_i(&rv, buf, 0xFFFFFFFF - i));
    }

    // Ensure new values fit properly
    for (int i = 0; i < maxI32; i++) { // floor(128 / (1 (header) + 1 (key) + 4 (data) + 1 (cache)))
        char buf[] = { 'a' + i, 0x00 };
        assert(rval_get_i(&rv, buf) == 0xFFFFFFFF - i);
    }

    printf("All storage test cases passed successfully!\n");

    return 0;
}