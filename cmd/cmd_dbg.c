#include "cmd_dbg.h"

void cmd_dbg_clr(cmd_t* cmd) {
    memset(cmd->buf.buf, 0, cmd->buf.cap);
    cmd->buf.cch_idx = 0;
    cmd->buf.chr_len = cmd->buf.cap - sizeof(cmd_cache_t);

    memset(cmd->entries.buf, 0, cmd->entries.cap * sizeof(cmd_entry_t)); // Clear command entry buffer

    cmd->last_cache = 0;
    cmd->state = CMD_RECV_OPEN;
}

void cmd_dbg_bbuf(cmd_t* cmd) {
    cmd_bbuf_ptr_t cap = cmd->buf.cap;

    printf("(%d) [ ", cap);
    for (cmd_bbuf_ptr_t i = 0; i < cap; i++) {
        uint8_t ch = cmd->buf.buf[i];

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '+' || ch == '-')
            printf("%c ", ch); // Printable character
        else if (ch == 0x00)
            printf("\\ ");       // Null byte
        else
            printf("%02X ", ch); // Print out as raw HEX
    }
    printf("]\n");
}