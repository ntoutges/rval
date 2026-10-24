/**
 * @file main.c
 * @author Nicholas T. (ntoutges@gmail.com)
 * @brief Test the `cmd` library and ensure that all features are working as intended
 * @version 1.0
 * @date 2026-05-21
 * 
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include <assert.h>
#include "../cmd.h"

uint8_t checksum = 0; // Proof that regCB was called
cmd_t myCmd;

// Callback to register
// Increment `checksum` by `base` + ogeti(0)
void regCB(void* base) {
    checksum += *((uint8_t*) base) + cmd_cogeti(1, 0);
}

#define EPSILON 0.0001

int main() {
    
    // Initialize buffers
    // cmd_entry_t entries[10];
    // uint8_t buf[64];

    // ======== TEST STATICALLY ALLOCATED CMD INSTANCE ========
    // myCmd = cmd('!' entries, sizeof(entries) / sizeof(*entries), buf, sizeof(buf) / sizeof(*buf), NULL);

    // ======== TEST DYNAMICALLY ALLOCATED CMD INSTANCE ========
    myCmd = cmd_f('!', 10, 64, NULL);

    // ========== TEST BASIC GETTERS ========
    cmd_recvs(&myCmd, "!test get 1.2 0 --a 10 --b 20 --c 30 -xyz\n");

    // Check ordered values
    assert(strcmp(cmd_ogets(&myCmd, 0, ""), "test") == 0);
    assert(strcmp(cmd_ogets(&myCmd, 1, ""), "get") == 0);
    assert(strcmp(cmd_ogets(&myCmd, 2, ""), "1.2") == 0);
    assert(strcmp(cmd_ogets(&myCmd, 3, ""), "0") == 0);

    // Check ordered value parsing
    assert(cmd_ogeti(&myCmd, 2, 0) == 1);
    assert(abs(cmd_ogetf(&myCmd, 2, 0) - 1.2) < EPSILON);
    assert(cmd_ogetb(&myCmd, 2, false) == true);
    assert(cmd_ogetb(&myCmd, 3, false) == false);

    // Check unordered values
    assert(strcmp(cmd_ugets(&myCmd, "a", ""), "10") == 0);
    assert(strcmp(cmd_ugets(&myCmd, "b", ""), "20") == 0);
    assert(strcmp(cmd_ugets(&myCmd, "c", ""), "30") == 0);

    // Check unordered parsing
    assert(cmd_ugeti(&myCmd, "a", 0) == 10);
    assert(cmd_ugeti(&myCmd, "b", 0) == 20);
    assert(cmd_ugeti(&myCmd, "c", 0) == 30);
    assert(cmd_ugetb(&myCmd, "a", false) == true);
    assert(cmd_ugetb(&myCmd, "b", false) == true);
    assert(cmd_ugetb(&myCmd, "c", false) == true);

    // Check special flag parsing
    assert(cmd_ugetb(&myCmd, "w", false) == false); // Not present: false
    assert(cmd_ugetb(&myCmd, "x", false) == true); // Present in -xyz block
    assert(cmd_ugetb(&myCmd, "y", false) == true); // Present in -xyz block
    assert(cmd_ugetb(&myCmd, "z", false) == true); // Present in -xyz block

    // ======== TEST COMMAND-SPECIFIC CALLBACKS ========

    // Add in callback
    uint8_t base = 1;
    uint8_t inc_id = cmd_attach(&myCmd, "inc", regCB, &base);

    cmd_recvs(&myCmd, "!inc 2\n");
    assert(checksum == 3); // 1 (base) + 2 = 3

    cmd_recvs(&myCmd, "!inc 3\n");
    assert(checksum == 7); // 3 + 1 (base) + 3 = 7

    cmd_recvs(&myCmd, "!nop 10\n");
    assert(checksum == 7); // Didn't increment

    cmd_recvs(&myCmd, "!inc 4\n");
    assert(checksum == 12); // 7 + 1 (base) + 4 = 12

    // ======== ENSURE CALLBACKS CAN BE DETACHED ========
    cmd_detach(&myCmd, inc_id);
    checksum = 0; // Reset checksum

    cmd_recvs(&myCmd, "!inc\n");
    assert(checksum == 0); // Should NOP now

    // ======== TEST "CURRENT" COMMANDS
    // Note: Intentionally reuses earlier testcase for easier isolation

    cmd_curr(&myCmd);
    cmd_recvs(&myCmd, "!test get 1.2 0 --a 10 --b 20 --c 30 -xyz\n");

    // Check ordered values
    assert(strcmp(cmd_cogets(0, ""), "test") == 0);
    assert(strcmp(cmd_cogets(1, ""), "get") == 0);
    assert(strcmp(cmd_cogets(2, ""), "1.2") == 0);
    assert(strcmp(cmd_cogets(3, ""), "0") == 0);

    // Check ordered value parsing
    assert(cmd_cogeti(2, 0) == 1);
    assert(abs(cmd_cogetf(2, 0) - 1.2) < EPSILON);
    assert(cmd_cogetb(2, false) == true);
    assert(cmd_cogetb(3, false) == false);

    // Check unordered values
    assert(strcmp(cmd_cugets("a", ""), "10") == 0);
    assert(strcmp(cmd_cugets("b", ""), "20") == 0);
    assert(strcmp(cmd_cugets("c", ""), "30") == 0);

    // Check unordered parsing
    assert(cmd_cugeti("a", 0) == 10);
    assert(cmd_cugeti("b", 0) == 20);
    assert(cmd_cugeti("c", 0) == 30);
    assert(cmd_cugetb("a", false) == true);
    assert(cmd_cugetb("b", false) == true);
    assert(cmd_cugetb("c", false) == true);

    // Check special flag parsing
    assert(cmd_cugetb("w", false) == false); // Not present: false
    assert(cmd_cugetb("x", false) == true); // Present in -xyz block
    assert(cmd_cugetb("y", false) == true); // Present in -xyz block
    assert(cmd_cugetb("z", false) == true); // Present in -xyz block

    // ======== FILL RX BUFFER ========
    cmd_recvs(&myCmd, "!a b c d e f g h i j k l m n o p\n"); // 32 bytes used by RX buf; 32 bytes used by cache

    for (char c = 'a'; c <= 'p'; c++) {
        assert(cmd_cogets(c - 'a', "-")[0] == c); // Ensure all bytes properly received+cached
    }

    // ======== OVERFILL RX BUFFER ========
    cmd_recvs(&myCmd, "!z y x w v u t s r q p o n m l k0\n"); // 33 bytes used by RX buf; 32 bytes used by cache

    for (char c = 'z'; c >= 'l'; c--) {
        assert(cmd_cogets('z' - c, "-")[0] == c); // Ensure all non-overflowing bytes properly received+cached
    }
    assert(cmd_cogets('z' - 'k', "-")[0] == '-'); // Ensure overflowing byte is ignored

    printf("All main test cases passed successfully!\n");

    return 0;
}
