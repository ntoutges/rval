#ifndef _CMD_H
#define _CMD_H

/**
 * @file cmd.h
 * @author Nicholas T.
 * @brief Command line argument parser
 * @version 0.1
 * @date 2026-04-07
 * 
 * @copyright PiCO 2026
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "cmd_str.h"

#ifdef __cplusplus
extern "C" {
#endif

// The size of the psuedo-pointer used to reference objects within
// the internal character RX/command index arena
#ifndef cmd_bbuf_ptr_t
#define cmd_bbuf_ptr_t uint8_t
#endif

/*
    Command Format:
    command: <initiator><command> <args>\n

    - <initiator>: A character that indicates a command was sent (eg: '!')
    - <command>: A string of characters that identifies the command (eg: 'help')
    - <args>: Arguments to run the command with. Format of arguments is (ordered ordered ... ordered --flag named --flag named)
              Note that single-character arguments are also supported under the -f format, and can be grouped together (eg: -abc is equivalent to -a -b -c)
*/

typedef struct cmd_entry_t {
    const char* ch;   // Command character(s) to trigger on
    void (*cb)(void* args); // Function to run on command receipt
    void* args; // Arguments to pass to the function when the command is triggered. The objected pointed to must remain valid when the command is triggered.
} cmd_entry_t;

typedef struct cmd_entries_t {
    cmd_entry_t* buf;   // Array of command entries
    uint8_t cap;        // Capacity of the command buffer
} cmd_entries_t;

typedef struct cmd_buf_t {
    uint8_t* buf;  // 

    cmd_bbuf_ptr_t cap; // Capacity of the command buffer

    cmd_bbuf_ptr_t chr_len; // Current character index in the command buffer (grows from 0 to cap-1)
    cmd_bbuf_ptr_t cch_idx; // Current character index in the command buffer (shrinks from cap-1 to 0)
} cmd_buf_t;

// Hold onto flag/value name
// If name = value: flag is ordered; Otherwise: flag is unordered
typedef struct cmd_cache_t {
    cmd_bbuf_ptr_t name; // Pointer to name of arg; If points to '-' character, indicates single-character ordered flag list
    uint8_t value; // Pointer offset from `name` to value of arg
} cmd_cache_t;

typedef enum cmd_recv_state_t {
    CMD_RECV_OPEN,      // Able to receive command characters
    CMD_RECV_CLOSED,    // Initator not found; Wait for next newline to reset
    CMD_RECV_COMMAND,   // Initator found; Receiving command characters
    CMD_RECV_COMMAND_E, // Waiting for character involved in escape sequence
    CMD_RECV_COMMAND_I,  // Inhibit new flag/value pairs: Receiving a single token
    CMD_RECV_COMMAND_IE, // Escape character within inhibited sequence
    CMD_RECV_FULL       // Receive buffer full
} cmd_recv_state_t;

typedef struct cmd_t {
    char initiator; // Character that indicates a command was sent (eg: '!')

    cmd_recv_state_t state; // Receive state of the command handler
    cmd_bbuf_ptr_t last_cache; // Index of the last cached character in the command buffer

    cmd_entries_t entries; // Command entries
    cmd_buf_t buf; // Buffer to hold current command character(s)

    void (*send)(char arg); // Transmit data to the other end of the `cmd` connection
} cmd_t;

/**
 * Create a new command handler
 * @param initiator     Character that indicates a command was sent (eg: '!<cmd>')
 * @param entry_buf     Buffer to hold command entries
 * @param entry_size    Size of the command entry buffer
 * @param buf_buf       Buffer to hold current command + cache
 * @param buf_size      Size of the buf_buf
 * @param send          A callback used to transmit some character
 * Set to `NULL` to make this a READ-ONLY `cmd` instance
 * @returns The allocated command handler
 */
cmd_t cmd(
    char initiator,
    cmd_entry_t* entry_buf,
    uint8_t entry_size,
    uint8_t* buf_buf,
    cmd_bbuf_ptr_t buf_size,
    void (*send)(char arg)
);

/**
 * Create a new command handler quickly (fast)
 * Intended to be used as a on-size-fits-all drop-in solution to ongoing projects
 * Uses dynamic memory allocation to minimize boilerplate required in getting this running,
 * yielding a larger memory footprint (and the possiblity of failure) with the benefit of
 * an easier dev experience
 * If either the entry buf or buf buf cannot be allocated, that buffer is reduced to a size of 0 B
 * @param initiator     Character that indicates a command was sent (eg: '!<cmd>')
 * @param entry_size    The number of commands to allocate space for
 * @param buf_size      The size of the buf_buf (receive + tag buffer)
 * @param send          A callback used to transmit some character
 * Set to `NULL` to make this a READ-ONLY `cmd` instance
 * @return The allocated command handler
 */
cmd_t cmd_f(
    char initiator,
    uint8_t entry_size,
    cmd_bbuf_ptr_t buf_size,
    void (*send)(char arg)
);

/**
 * Mark the given `cmd` instance as the "current" instance
 * This will persist until some command instance, in `cmd_recv`, successfully finishes a command
 * Set to `NULL` to reset or indicate that the `cmd` instance is being retired
 * 
 * @param cmd   The `cmd` instance to mark as "current"
 */
void cmd_curr(cmd_t* cmd);

/**
 * Trigger on receiving a command character
 * @param cmd   The command handler to trigger on; Pass in NULL to use the "current" command handler. If none exist, NOPs and returns false
 * @param ch    The received command character
 * @returns     true if the character was part of a command, false otherwise
 */
bool cmd_recv(cmd_t* cmd, char ch);

/**
 * Trigger on receiving a command string
 * @param cmd   The command handler to trigger on; Pass in NULL to use the "current" command handler. If none exist, NOPs and returns false
 * @param str   The received command string to process
 * @returns     true if some part of the string was part of a command, false otherwise
 */
bool cmd_recvs(cmd_t* cmd, const char* str);

/**
 * Send a single character across the `cmd` bridge
 * @param cmd   The command handler to trigger. Pass in NULL to use the "current" command handler. If none exist, NOPs and returns false
 * @param ch    The character to send. If 0x00, sends the initiator character (signaling the start of a transmission)
 * @returns     true if the character was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_send(cmd_t* cmd, char ch);

/**
 * Send a full string across the `cmd` bridge
 * @param cmd   The command handler to trigger. Pass in NULL to use the "current" command handler. If none exist, NOPs and returns false
 * @param str   The null-terminated string to send. Note that this function does _not_ automatically add a `\n` character
 * @returns     true if the string was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_sends(cmd_t* cmd, const char* str);

/**
 * Send a raw string across the `cmd` bridge
 * @param cmd   The command handler to trigger. Pass in NULL to use the "current" command handler. If none exist, NOPs and returns false
 * @param str   The null-terminated string to send. Note that this function does _not_ automatically add a `\n` character
 * Escapes special characters '"' and '\'
 * @returns     true if the string was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_sendr(cmd_t* cmd, const char* str);

/**
 * Send a full integer across the `cmd` bridge
 * @param cmd   The command handler to trigger. Pass in NULL to use the "current" command handler. If none exist, NOPs and returns false
 * @param i     The integer to send. Note that this function does _not_ automatically add a `\n` character
 * @returns     true if the int was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_sendi(cmd_t* cmd, int32_t i);

/**
 * Send a full float across the `cmd` bridge
 * @param cmd   The command handler to trigger. Pass in NULL to use the "current" command handler. If none exist, NOPs and returns false
 * @param f     The integer to send. Note that this function does _not_ automatically add a `\n` character
 * @returns     true if the float was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_sendf(cmd_t* cmd, float f);

/**
 * Attempt to attach a command entry point to the command handler
 * @param cmd Command handler to attach to; Pass in NULL to use the "current" command handler. If none exist, NOPs and returns 0xFF
 * @param command Command string (0th ordered argument) to trigger on (eg: "help")
 * @param cb Function to run when the command is triggered
 * @param args Arguments to pass to the function when the command is triggered. The objected pointed to must remain valid when the command is triggered.
 * @returns The id of the attached command entry, or 0xFF on failure (eg: command buffer full, command already exists, etc.)
 */
uint8_t cmd_attach(cmd_t* cmd, const char* command, void (*cb)(void* args), void* args);

/**
 * Attempt to detach a command entry point from the command handler
 * @param cmd Command handler to detach from; Pass in NULL to use the "current" command handler. If none exist, NOPs and returns 0xFF
 * @param id ID of the command entry to detach
 * @returns The id of the detached command entry, or 0xFF on failure
 */
uint8_t cmd_detach(cmd_t* cmd, uint8_t id);

/**
 * Attempt to grab an integer value from the unordered command cache
 * @param cmd Command to grab from; Pass in NULL to use the "current" command handler. If none exist, returns default value
 * @param name Name of the argument to grab
 * @param default_val Value to return if the argument is not found
 */
int cmd_ugeti(cmd_t* cmd, const char* name, int default_val);

/**
 * Attempt to grab a float value from the unordered command cache
 * @param cmd Command to grab from; Pass in NULL to use the "current" command handler. If none exist, returns default value
 * @param name Name of the argument to grab
 * @param default_val Value to return if the argument is not found
 */
float cmd_ugetf(cmd_t* cmd, const char* name, float default_val);

/**
 * Attempt to grab a boolean value from the unordered command cache
 * @param cmd Command to grab from; Pass in NULL to use the "current" command handler. If none exist, returns default value
 * @param name Name of the argument to grab
 * @param default_val Value to return if the argument is not found
 */
bool cmd_ugetb(cmd_t* cmd, const char* name, bool default_val);

/**
 * Attempt to grab a boolean value from the ordered command cache
 * @param cmd Command to grab from; Pass in NULL to use the "current" command handler. If none exist, returns default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Default string value to copy into buffer if argument is not found
 */
const char* cmd_ugets(cmd_t* cmd, const char* name, const char* default_val);

/**
 * Attempt to grab an integer value from the ordered command cache
 * @param cmd Command to grab from; Pass in NULL to use the "current" command handler. If none exist, returns default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Value to return if the argument is not found
 */
int cmd_ogeti(cmd_t* cmd, uint8_t idx, int default_val);

/**
 * Attempt to grab a float value from the ordered command cache
 * @param cmd Command to grab from; Pass in NULL to use the "current" command handler. If none exist, returns default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Value to return if the argument is not found
 */
float cmd_ogetf(cmd_t* cmd, uint8_t idx, float default_val);

/**
 * Attempt to grab a boolean value from the ordered command cache
 * @param cmd Command to grab from; Pass in NULL to use the "current" command handler. If none exist, returns default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Value to return if the argument is not found
 */
bool cmd_ogetb(cmd_t* cmd, uint8_t idx, bool default_val);

/**
 * Attempt to grab a boolean value from the ordered command cache
 * @param cmd Command to grab from; Pass in NULL to use the "current" command handler. If none exist, returns default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Default string value to copy into buffer if argument is not found
 */
const char* cmd_ogets(cmd_t* cmd, uint8_t idx, const char* default_val);


// ======== CURRENT SIMPLIFICATIONS OF CMD OPERATIONS ========

/**
 * Trigger on receiving a command character using the current command instance
 * If no current command is registered, this NOPs and returns `false`
 * @param ch The received command character
 * @returns     true if the character was part of a command, false otherwise
 */
bool cmd_crecv(char ch);

/**
 * Trigger on receiving a command string using the current command instance
 * @param str   The received command string to process
 * @returns     true if some part of the string was part of a command, false otherwise
 */
bool cmd_crecvs(const char* str);

/**
 * Send a single character across the `cmd` bridge using the current command instance
 * If no current command is registered, this NOPs and returns `false`
 * @param ch    The character to send. If 0x00, sends the initiator character (signaling the start of a transmission)
 * @returns     true if the character was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_csend(char ch);

/**
 * Send a full string across the `cmd` bridge using the current command instance
 * If no current command is registered, this NOPs and returns `false`
 * @param str   The null-terminated string to send. Note that this function does _not_ automatically add a `\n` character
 * @returns     true if the string was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_csends(const char* str);

/**
 * Send a raw string across the `cmd` bridge using the current command instance
 * If no current command is registered, this NOPs and returns `false`
 * @param str   The null-terminated string to send. Note that this function does _not_ automatically add a `\n` character
 * Escapes special characters '"' and '\'
 * @returns     true if the string was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_csendr(const char* str);

/**
 * Send a full integer across the `cmd` bridge using the current command instance
 * If no current command is registered, this NOPs and returns `false`
 * @param i     The integer to send. Note that this function does _not_ automatically add a `\n` character
 * @returns     true if the int was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_csendi(cmd_t* cmd, int32_t i);

/**
 * Send a full float across the `cmd` bridge using the current command instance
 * If no current command is registered, this NOPs and returns `false`
 * @param f     The integer to send. Note that this function does _not_ automatically add a `\n` character
 * @returns     true if the float was sent (command handler exists and is not read-only), false otherwise
 */
bool cmd_csendf(cmd_t* cmd, float f);

/**
 * Attempt to attach a command entry point to the command handler using the current command instance
 * If no current command is registered, this NOPs and returns 0xFF
 * @param command Command string (0th ordered argument) to trigger on (eg: "help")
 * @param cb Function to run when the command is triggered
 * @param args Arguments to pass to the function when the command is triggered. The objected pointed to must remain valid when the command is triggered.
 * @returns The id of the attached command entry, or 0xFF on failure (eg: command buffer full, command already exists, etc.)
 */
uint8_t cmd_cattach(const char* command, void (*cb)(void* args), void* args);

/**
 * Attempt to detach a command entry point from the command handler using the current command instance
 * If no current command is registered, this NOPs and returns 0xFF
 * @param id ID of the command entry to detach
 * @returns The id of the detached command entry, or 0xFF on failure
 */
uint8_t cmd_cdetach(uint8_t id);

/**
 * Attempt to grab an integer value from the unordered command cache using the current command
 * If no current command is registered, this NOPs and returns the default value
 * @param name Name of the argument to grab
 * @param default_val Value to return if the argument is not found
 */
int cmd_cugeti(const char* name, int default_val);

/**
 * Attempt to grab a float value from the unordered command cache using the current command
 * If no current command is registered, this NOPs and returns the default value
 * @param name Name of the argument to grab
 * @param default_val Value to return if the argument is not found
 */
float cmd_cugetf(const char* name, float default_val);

/**
 * Attempt to grab a boolean value from the unordered command cache using the current command
 * If no current command is registered, this NOPs and returns the default value
 * @param name Name of the argument to grab
 * @param default_val Value to return if the argument is not found
 */
bool cmd_cugetb(const char* name, bool default_val);

/**
 * Attempt to grab a boolean value from the ordered command cache using the current command
 * If no current command is registered, this NOPs and returns the default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Default string value to copy into buffer if argument is not found
 */
const char* cmd_cugets(const char* name, const char* default_val);

/**
 * Attempt to grab an integer value from the ordered command cache using the current command
 * If no current command is registered, this NOPs and returns the default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Value to return if the argument is not found
 */
int cmd_cogeti(uint8_t idx, int default_val);

/**
 * Attempt to grab a float value from the ordered command cache using the current command
 * If no current command is registered, this NOPs and returns the default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Value to return if the argument is not found
 */
float cmd_cogetf(uint8_t idx, float default_val);

/**
 * Attempt to grab a boolean value from the ordered command cache using the current command
 * If no current command is registered, this NOPs and returns the default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Value to return if the argument is not found
 */
bool cmd_cogetb(uint8_t idx, bool default_val);

/**
 * Attempt to grab a boolean value from the ordered command cache using the current command
 * If no current command is registered, this NOPs and returns the default value
 * @param idx Index of the argument to grab (0-indexed)
 * @param default_val Default string value to copy into buffer if argument is not found
 */
const char* cmd_cogets(uint8_t idx, const char* default_val);

#ifdef __cplusplus
}
#endif

#endif