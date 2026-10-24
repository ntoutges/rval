/**
 * @file tx.c
 * @author Nicholas T. (ntoutges@gmail.com)
 * @brief Test transmission
 * @version 1.0
 * @date 2026-05-21
 * 
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include <assert.h>
#include "../cmd.h"

cmd_t cmdA, cmdB;

void ping(void* args) {
    cmd_csend(0x00);
    cmd_csends("pong --data \"");
    cmd_csends(cmd_cugets("data", ""));
    cmd_csends("\"\n");
}

void ping_resp(char ch) {
    cmd_recv(&cmdB, ch);
}

char pong_data[100] = { 0x00 };
void pong(void* args) {
    const char* data = cmd_cugets("data", "");

    // Copy data into pong_data (safely)
    int length = strnlen(data, sizeof(pong_data) - 1);
    memcpy(pong_data, data, length);
    pong_data[length] = 0x00; // Add in null byte (force safety)
}

int main() {

    // ======== TEST DYNAMICALLY ALLOCATED CMD INSTANCE ========
    cmdA = cmd_f('!', 1, 64, ping_resp);
    cmdB = cmd_f('!', 1, 64, NULL);

    cmd_attach(&cmdA, "ping", ping, NULL);
    cmd_attach(&cmdB, "pong", pong, NULL);
    
    // ======== Trigger TX Command ========
    cmd_recvs(&cmdA, "!ping --data \"command to send\"\n");
    // ... Expect `pong` response on cmdB...
    // ping (resp) => ping (fn) => send (internal) => ping_resp (fn) => [assign pong_data]

    assert(strcmp(pong_data, "command to send") == 0);

    printf("All tx test cases passed successfully!\n");

    return 0;
}