#include "cmd.h"

bool _cmd_update_cache(cmd_t* cmd); // Attempt to update cache based on newly received character in command buffer; Returns `false` on error
uint8_t _cmd_find_ucache(cmd_t* cmd, const char* name); // Search for a cache entry with the given name; Returns index of cache entry if found, 0xFF otherwise
uint8_t _cmd_find_ocache(cmd_t* cmd, uint8_t idx); // Search for a cache entry with the given index; Returns index of cache entry if found, 0xFF otherwise
uint8_t _cmd_find_fcache(cmd_t* cmd, uint8_t start); // Search for the next single-character flag cache entry starting from the given index; Returns index of cache entry if found, 0xFF otherwise
cmd_cache_t _cmd_read_cache(cmd_t* cmd, uint8_t cache_idx); // Read a cache entry at the given index; Returns the cache entry
uint8_t _cmd_cache_len(cmd_t* cmd); // Get the length of the command cache (number of entries)

// Keep track of the last referenced `cmd` instance
// Within+after a registered callback, this will be the `cmd` that triggered the instance
// Set whenever a `cmd_recv` successfully decodes a command
cmd_t* _cmd_current_cmd = NULL;

cmd_t cmd(
    char initiator,
    cmd_entry_t* entry_buf,
    uint8_t entry_size,
    uint8_t* buf_buf,
    cmd_bbuf_ptr_t buf_size,
    void (*send)(char arg)
) {
    memset(entry_buf, 0, entry_size * sizeof(cmd_entry_t)); // Clear command entry buffer

    return (cmd_t) {
        .initiator = initiator,
        .state = CMD_RECV_OPEN,
        .last_cache = 0,
        .entries = (cmd_entries_t) {
            .buf = entry_buf,
            .cap = entry_size,
        },
        .buf = (cmd_buf_t) {
            .buf = buf_buf,
            .cap = buf_size,
            .chr_len = 0,
            .cch_idx = buf_size - sizeof(cmd_cache_t)
        },
        .send = send
    };
}

cmd_t cmd_f(
    char initiator,
    uint8_t entry_size,
    cmd_bbuf_ptr_t buf_size,
    void (*send)(char arg)
) {
    cmd_entry_t* entry_buf = (cmd_entry_t*) malloc(entry_size * sizeof(cmd_entry_t));
    uint8_t* buf_buf = (uint8_t*) malloc(buf_size * sizeof(uint8_t));

    if (entry_buf == NULL) entry_size = 0; // Failed to allocate entries
    if (buf_buf == NULL) buf_size = 0; // Failed to allocate main buf

    // Create `cmd` using the allocated buffers
    return cmd(initiator, entry_buf, entry_size, buf_buf, buf_size, send);
}

void cmd_curr(cmd_t* cmd) {
    _cmd_current_cmd = cmd;
}

bool cmd_recv(cmd_t* cmd, char ch) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return false;           // Vacuously unused

    if (ch == '\r') return false; // Ignore the evil character

    if (ch == '\n') {
        if (cmd->state == CMD_RECV_COMMAND) {
            // Newline (not preceded by space) indicates end of command token; Attempt to analyze and cache new token
            if ((cmd->buf.chr_len > 0 && cmd->buf.buf[cmd->buf.chr_len - 1] != ' ')) {

                // Command interpretation failed; Move to closed state
                if (!_cmd_update_cache(cmd)) cmd->state = CMD_RECV_CLOSED;
            }
        }

        bool char_used = false;
        if (cmd->state == CMD_RECV_COMMAND || cmd->state == CMD_RECV_COMMAND_I) {
            _cmd_current_cmd = cmd; // Update current command on successful decoding

            // Attempt to run associated command
            const char* command_buf = cmd_ogets(cmd, 0, ""); // Ensure cache is updated with all received tokens before running command callback

            for (uint8_t i = 0; i < cmd->entries.cap; i++) {
                if (cmd->entries.buf[i].ch != NULL && strcmp(cmd->entries.buf[i].ch, command_buf) == 0) {
                    cmd->entries.buf[i].cb(cmd->entries.buf[i].args); // Found matching command entry; Run callback with associated arguments
                    break;
                }
            }

            char_used = true;
        }

        // Reset command state on newline
        cmd->state = CMD_RECV_OPEN;
        return char_used;
    }

    switch (cmd->state) {
        case CMD_RECV_OPEN:
            if (ch == cmd->initiator) {
                cmd->state = CMD_RECV_COMMAND;

                // Reset cache and command buffer indices to prepare for new command
                cmd->last_cache = 0;
                cmd->buf.chr_len = 0;
                cmd->buf.cch_idx = cmd->buf.cap - sizeof(cmd_cache_t);
                return true;
            }
            cmd->state = CMD_RECV_CLOSED;
            return false;

        case CMD_RECV_CLOSED:
            return false;

        case CMD_RECV_COMMAND:
        case CMD_RECV_COMMAND_E:
        case CMD_RECV_COMMAND_I:
        case CMD_RECV_COMMAND_IE:
            
            // Not currently in escape mode
            // Process special characters
            if (
                cmd->state != CMD_RECV_COMMAND_E
                && cmd->state != CMD_RECV_COMMAND_IE
            ) {

                // Move to escape mode if we see a '\' character
                if (ch == '\\') {
                    cmd->state = cmd->state == CMD_RECV_COMMAND
                        ? CMD_RECV_COMMAND_E
                        : CMD_RECV_COMMAND_IE;
                    return true;
                }

                // Quote indicates start/end of an inhibited tokens
                if (ch == '"') {
                    cmd->state = cmd->state == CMD_RECV_COMMAND
                        ? CMD_RECV_COMMAND_I
                        : CMD_RECV_COMMAND;
                    return true;
                }
            }
            
            // Move out of escape mode
            else {
                cmd->state = cmd->state == CMD_RECV_COMMAND_E
                    ? CMD_RECV_COMMAND
                    : CMD_RECV_COMMAND_I;
            }

            // Not currently in inhibit mode
            // Process spaces as normal
            if (cmd->state != CMD_RECV_COMMAND_I) {

                // New space indicates end of command token; Attempt to analyze and cache new token
                if (ch == ' ' && (cmd->buf.chr_len > 0 && cmd->buf.buf[cmd->buf.chr_len - 1] != ' ')) {

                    // Cache update failed; Drop command and wait for newline to reset
                    if (!_cmd_update_cache(cmd)) {
                        cmd->state = CMD_RECV_FULL;
                    }

                    return true;
                }
            }

            // Plain character; Process as normal
            if (cmd->buf.chr_len <= cmd->buf.cch_idx) {
                cmd->buf.buf[cmd->buf.chr_len++] = ch;
                return true;
            }

            // Buffer overflow; Drop command and wait for newline to reset
            cmd->state = CMD_RECV_FULL;
            return true;

        case CMD_RECV_FULL:
            // Buffer overflow; Drop command and wait for newline to reset
            return true;
    }

    // How did you get here!?
    return false;
}

/**
 * Trigger on receiving a command string
 * @param cmd   The command handler to trigger on
 * @param str   The received command string to process
 * @returns     true if some part of the string was part of a command, false otherwise
 */
bool cmd_recvs(cmd_t* cmd, const char* str) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return false;           // Vacuously unused

    bool used = false;

    // Run through receiving all characters one-by-one
    for (uint32_t i = 0; str[i]; i++) {
        used = cmd_recv(cmd, str[i]) || used;
    }

    // Return if the string was useful
    return used;
}

bool cmd_send(cmd_t* cmd, char ch) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return false;           // Return failure code
    if (cmd->send == 0x00) return false;     // Read-only; Fail!

    if (ch == 0x00) ch = cmd->initiator; // Fill in initiator character

    // Send character
    cmd->send(ch);

    return true;
}

bool cmd_sends(cmd_t* cmd, const char* str) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return false;           // Return failure code
    if (cmd->send == 0x00) return false;     // Read-only; Fail!

    // Send full string
    for (uint32_t i = 0; str[i]; i++) {
        cmd->send(str[i]);
    }

    return true;
}

bool cmd_sendr(cmd_t* cmd, const char* str) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return false;           // Return failure code
    if (cmd->send == 0x00) return false;     // Read-only; Fail!

    // Send full string
    for (uint32_t i = 0; str[i]; i++) {
        char ch = str[i];

        // Prepend with escape if required
        if (ch == '"' || ch == '\\') {
            cmd->send('\\');
        }

        cmd->send(ch);
    }

    return true;
}

bool cmd_sendi(cmd_t* cmd, int32_t i) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return false;           // Return failure code
    if (cmd->send == 0x00) return false;     // Read-only; Fail!

    // Convert to string
    char buf[12]; // Maximum size required by int
    cmd_str_itoa(i, buf, sizeof(buf));

    // Send converted string
    cmd_send(cmd, '+');
    cmd_sends(cmd, buf);

    return true;
}

bool cmd_sendf(cmd_t* cmd, float f) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return false;           // Return failure code
    if (cmd->send == 0x00) return false;     // Read-only; Fail!

    // Convert to string
    char buf[16]; // Comfortable number of digits
    cmd_str_ftoma(f, buf, sizeof(buf));

    // Send converted string
    cmd_send(cmd, '+');
    cmd_sends(cmd, buf);

    return true;
}

uint8_t cmd_attach(cmd_t* cmd, const char* command, void (*cb)(void* args), void* args) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return 0xFF;            // Return failure code

    // Search for an empty slot in the command entry buffer
    for (uint8_t i = 0; i < cmd->entries.cap; i++) {
        if (cmd->entries.buf[i].ch == NULL) { // Found empty slot
            cmd->entries.buf[i] = (cmd_entry_t) {
                .ch = command,
                .cb = cb,
                .args = args
            };
            return i; // Return id of attached command entry
        }
    }

    return 0xFF; // No empty slot found; Return failure code
}

uint8_t cmd_detach(cmd_t* cmd, uint8_t id) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return 0xFF;            // Return failure code

    if (id >= cmd->entries.cap) return 0xFF; // Invalid id; Return failure code

    cmd->entries.buf[id].ch = NULL; // Mark command entry as empty
    return id; // Return id of detached command entry
}

int cmd_ugeti(cmd_t* cmd, const char* name, int default_val) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return default_val;     // Return failure code

    uint8_t cidx = _cmd_find_ucache(cmd, name);
    if (cidx == 0xFF) return default_val; // Cache entry not found; Return default value

    cmd_cache_t cache = _cmd_read_cache(cmd, cidx);
    return cmd_str_atoi((char*) &cmd->buf.buf[cache.name + cache.value]);
}

float cmd_ugetf(cmd_t* cmd, const char* name, float default_val) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return default_val;     // Return failure code

    uint8_t cidx = _cmd_find_ucache(cmd, name);
    if (cidx == 0xFF) return default_val; // Cache entry not found; Return default value

    cmd_cache_t cache = _cmd_read_cache(cmd, cidx);
    return cmd_str_atof((char*) &cmd->buf.buf[cache.name + cache.value]);
}

bool cmd_ugetb(cmd_t* cmd, const char* name, bool default_val) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return default_val;     // Return failure code

    uint8_t cidx = 0xFF;
    
    // Look for single-character unordered flag
    if (strlen(name) == 1) {
        uint8_t last = 0;

        while (1) {
            cidx = _cmd_find_fcache(cmd, last);
            if (cidx == 0xFF) break; // No more single-character flags in cache; Stop searching

            cmd_cache_t cache = _cmd_read_cache(cmd, cidx);

            // Check if any of the characters in the flag value match the given name character
            uint8_t i = 0;
            cmd_bbuf_ptr_t base = cache.name + cache.value;
            while (cmd->buf.buf[base + i] != 0x00) {
                if (cmd->buf.buf[base + i] == name[0]) return true; // Found flag character in cache; Return true
                i++;
            }

            // Increment to check next single-character flag in cache
            last = cidx + 1;
        }
    }

    // Fallback to standard unordered flag lookup for non-single-character flags
    cidx = _cmd_find_ucache(cmd, name);
    if (cidx == 0xFF) return default_val; // Cache entry not found; Return default value

    cmd_cache_t cache = _cmd_read_cache(cmd, cidx);
    return cmd_str_atoi((char*) &cmd->buf.buf[cache.name + cache.value]) != 0;
}

const char* cmd_ugets(cmd_t* cmd, const char* name, const char* default_val) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return default_val;     // Return failure code
    
    uint8_t cidx = _cmd_find_ucache(cmd, name);
    if (cidx == 0xFF) {
        return default_val;
    }

    cmd_cache_t cache = _cmd_read_cache(cmd, cidx);
    return (const char*) &cmd->buf.buf[cache.name + cache.value];
}

int cmd_ogeti(cmd_t* cmd, uint8_t idx, int default_val) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return default_val;     // Return failure code

    uint8_t cidx = _cmd_find_ocache(cmd, idx);
    if (cidx == 0xFF) return default_val; // Cache entry not found; Return default value

    cmd_cache_t cache = _cmd_read_cache(cmd, cidx);
    return cmd_str_atoi((char*) &cmd->buf.buf[cache.name + cache.value]);
}

float cmd_ogetf(cmd_t* cmd, uint8_t idx, float default_val) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return default_val;     // Return failure code
    
    uint8_t cidx = _cmd_find_ocache(cmd, idx);
    if (cidx == 0xFF) return default_val; // Cache entry not found; Return default value

    cmd_cache_t cache = _cmd_read_cache(cmd, cidx);
    return cmd_str_atof((char*) &cmd->buf.buf[cache.name + cache.value]);
}

bool cmd_ogetb(cmd_t* cmd, uint8_t idx, bool default_val) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return default_val;     // Return failure code

    uint8_t cidx = _cmd_find_ocache(cmd, idx);
    if (cidx == 0xFF) return default_val; // Cache entry not found; Return default value

    cmd_cache_t cache = _cmd_read_cache(cmd, cidx);
    return cmd_str_atoi((char*) &cmd->buf.buf[cache.name + cache.value]) != 0;
}

const char* cmd_ogets(cmd_t* cmd, uint8_t idx, const char* default_val) {
    if (cmd == NULL) cmd = _cmd_current_cmd; // Grab default
    if (cmd == NULL) return default_val;     // Return failure code

    uint8_t cidx = _cmd_find_ocache(cmd, idx);
    if (cidx == 0xFF) {
        return default_val;
    }

    cmd_cache_t cache = _cmd_read_cache(cmd, cidx);
    return (const char*) &cmd->buf.buf[cache.name + cache.value];
}

// Block of "Current" operations
// Note that these are just shallow wrappers around the normal commands
bool cmd_crecv(char ch)                                                         { return cmd_recv(_cmd_current_cmd, ch); }
bool cmd_crecvs(const char* str)                                                { return cmd_recvs(_cmd_current_cmd, str); }
bool cmd_csend(char ch)                                                         { return cmd_send(_cmd_current_cmd, ch); }
bool cmd_csends(const char* str)                                                { return cmd_sends(_cmd_current_cmd, str); }
bool cmd_csendr(const char* str)                                                { return cmd_sendr(_cmd_current_cmd, str); }
bool cmd_csendi(cmd_t* cmd, int32_t i)                                          { return cmd_sendi(_cmd_current_cmd, i); }
bool cmd_csendf(cmd_t* cmd, float f)                                            { return cmd_sendf(_cmd_current_cmd, f); }
uint8_t cmd_cattach(const char* command, void (*cb)(void* args), void* args)    { return cmd_attach(_cmd_current_cmd, command, cb, args); }
uint8_t cmd_cdetach(uint8_t id)                                                 { return cmd_detach(_cmd_current_cmd, id); }
int cmd_cugeti(const char* name, int default_val)                   { return cmd_ugeti(NULL, name, default_val); }
float cmd_cugetf(const char* name, float default_val)               { return cmd_ugetf(NULL, name, default_val); }
bool cmd_cugetb(const char* name, bool default_val)                 { return cmd_ugetb(NULL, name, default_val); }
const char* cmd_cugets(const char* name, const char* default_val)   { return cmd_ugets(NULL, name, default_val); }
int cmd_cogeti(uint8_t idx, int default_val)                    { return cmd_ogeti(NULL, idx, default_val); }
float cmd_cogetf(uint8_t idx, float default_val)                { return cmd_ogetf(NULL, idx, default_val); }
bool cmd_cogetb(uint8_t idx, bool default_val)                  { return cmd_ogetb(NULL, idx, default_val); }
const char* cmd_cogets(uint8_t idx, const char* default_val)    { return cmd_ogets(NULL, idx, default_val); }


// PRIVATE FUNCTIONS

bool _cmd_update_cache(cmd_t* cmd) {
    
    // Place null byte at the end of the current token to indicate end of string
    if (cmd->buf.chr_len < cmd->buf.cch_idx)
        cmd->buf.buf[cmd->buf.chr_len++] = 0x00;
    else return false; // Out of space in buffer

    cmd_cache_t cache;

    // Advance last_cache index to start of token (ignore any leading spaces)
    while (cmd->last_cache < cmd->buf.chr_len - 1 && cmd->buf.buf[cmd->last_cache] == ' ') {
        cmd->last_cache++;
    }

    // Token is raw value; Assume unordered value
    // Indicated by lack of leading '-' character
    if (cmd->buf.buf[cmd->last_cache] != '-') {

        if (cmd->last_cache >= cmd->buf.cch_idx - 1) {
            return false; // Out of space in buffer
        }

        // Cache new token as unordered value
        cache.name = cmd->last_cache;
        cache.value = 0;

        // Copy new cache entry to end of buffer, growing backwards from the end
        memcpy(&cmd->buf.buf[cmd->buf.cch_idx], &cache, sizeof(cmd_cache_t));
        cmd->buf.cch_idx -= sizeof(cmd_cache_t);
        cmd->last_cache = cmd->buf.chr_len;

        return true;
    }

    // Ensure token is long enough to be some valid flag
    if (cmd->last_cache + 1 >= cmd->buf.chr_len) {
        cmd->buf.chr_len = cmd->last_cache; // Drop invalid from buffer
        return true; // String too short to be a valid flag; However, not out of memory
    }

    // Token is single-character flag; Assume ordered flags
    // Indicated by a single leading '-' character followed by some number of characters
    if (cmd->buf.buf[cmd->last_cache + 1] != '-') {

        if (cmd->last_cache >= cmd->buf.cch_idx - 1) return false; // Out of space in buffer

        // Cache new token as ordered flag with empty value
        cache.name = cmd->last_cache; // Name pointing to '-' character indicates ordered single-char flag
        cache.value = 1;              // Value is all values after '-' character

        // Copy new cache entry to end of buffer, growing backwards from the end
        memcpy(&cmd->buf.buf[cmd->buf.cch_idx], &cache, sizeof(cmd_cache_t));
        cmd->buf.cch_idx -= sizeof(cmd_cache_t);
        cmd->last_cache = cmd->buf.chr_len;
        return true;
    }

    // Looking at multi-character flag, in the format of '--flag value'
    // Note that this requires 2 tokens (--flag, value)
    // Ensure that >= 2 tokens are present; Otherwise: wait for more tokens to arrive
    cmd_bbuf_ptr_t first_token_idx = cmd->last_cache + 2; // Skip past '--' characters
    cmd_bbuf_ptr_t second_token_idx = first_token_idx;

    // Search for the second token by looking for the next null byte before the end of the buffer
    while (++second_token_idx < cmd->buf.chr_len - 1 && cmd->buf.buf[second_token_idx] != 0x00);
    second_token_idx++; // Move past null byte to start of second token

    // Reached end of buffer without finding second token; Wait for more tokens to arrive
    if (second_token_idx == cmd->buf.chr_len) {
        return true;
    }

    if (cmd->last_cache >= cmd->buf.cch_idx - 1) return false; // Out of space in buffer

    // Remove initial "--" from flag to save space
    memmove(&cmd->buf.buf[cmd->last_cache], &cmd->buf.buf[first_token_idx], cmd->buf.chr_len - first_token_idx);
    first_token_idx -= 2;
    second_token_idx -= 2;


    // Cache new token as ordered flag with value
    cache.name = first_token_idx;
    cache.value = second_token_idx - first_token_idx;
    cmd->buf.chr_len -= 2; // Account for removed "--" characters

    // Copy new cache entry to end of buffer, growing backwards from the end
    memcpy(&cmd->buf.buf[cmd->buf.cch_idx], &cache, sizeof(cmd_cache_t));
    cmd->buf.cch_idx -= sizeof(cmd_cache_t);
    cmd->last_cache = cmd->buf.chr_len;

    return true;
}

// Look for any unordered flag cache entries (name != value) with the given name.
uint8_t _cmd_find_ucache(cmd_t* cmd, const char* name) {
    uint8_t cache_entries = _cmd_cache_len(cmd);

    for (uint8_t i = 0; i < cache_entries; i++) {
        cmd_cache_t cache = _cmd_read_cache(cmd, i);

        if (
            cache.name + cache.value < cmd->buf.cap &&
            cache.value != 0 && // name != value (value offset is not 0) indicates unordered flag; Skip ordered flags
            cmd->buf.buf[cache.name] != '-' && // '-' character in name indicates ordered single-character flag, not an unordered flag; Skip these entries
            memcmp(&cmd->buf.buf[cache.name], name, strlen(name) + 1) == 0
        ) {
            return i;
        }
    }

    return 0xFF;
}

// Look for any ordered flag cache (name = value) entries.
// Returns the `idx`th entry
uint8_t _cmd_find_ocache(cmd_t* cmd, uint8_t idx) {
    uint8_t cache_entries = _cmd_cache_len(cmd);
    uint8_t found_flags = 0;

    for (uint8_t i = 0; i < cache_entries; i++) {
        cmd_cache_t cache = _cmd_read_cache(cmd, i);

        if (
            cache.name + cache.value < cmd->buf.cap &&
            cache.value == 0 && // name = value (value offset is 0) indicates ordered flag
            found_flags++ == idx
        ) {
            return i;
        }
    }

    return 0xFF;
}

// Look for any unordered flag cache entries starting from the given index
// Return the first whose `name` points to a '-' character
uint8_t _cmd_find_fcache(cmd_t* cmd, uint8_t start) {
    uint8_t cache_entries = _cmd_cache_len(cmd);

    for (uint16_t i = start; i < cache_entries; i++) {
        cmd_cache_t cache = _cmd_read_cache(cmd, (uint8_t) i);

        if (cache.name < cmd->buf.cap && cmd->buf.buf[cache.name] == '-') {
            return i;
        }
    }

    return 0xFF; // No matching cache entry found
}


cmd_cache_t _cmd_read_cache(cmd_t* cmd, uint8_t cache_idx) {
    // Cache index out of bounds; Return empty cache
    if (cache_idx >= _cmd_cache_len(cmd))
        return (cmd_cache_t) { 0x00, 0x00 };

    cmd_cache_t cache;
    memcpy(&cache, &cmd->buf.buf[cmd->buf.cap - (cache_idx + 1) * sizeof(cmd_cache_t)], sizeof(cmd_cache_t));

    return cache;
}

uint8_t _cmd_cache_len(cmd_t* cmd) {
    return (cmd->buf.cap - cmd->buf.cch_idx) / sizeof(cmd_cache_t) - 1;
}
