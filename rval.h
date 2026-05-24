#ifndef __RVAL_H
#define __RVAL_H

/**
 * @file rval.h
 * @author Nicholas T.
 * @brief Local/Remote value synchronization
 * @version 0.1
 * @date 2026-05-24
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <stdint.h>
#include <string.h>
#include "cmd/cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

// The size of the psuedo-pointer used to reference objects within
// the internal value buffer
#ifndef rval_bbuf_ptr_t
#define rval_bbuf_ptr_t uint8_t
#endif

/*
    Buffer format:
    Dual-headed

    [ ENTRY ENTRY ENTRY ----->     <----- CACHE CACHE CACHE ]
    
    Each CACHE entry is a `rval_bbuf_ptr_t`, pointing to some `ENTRY` header

    Each `ENTRY` is of the following format:
    [ HEADER KEY VALUE ]

    HEADER is a 1 B value containing:
    - 3 b => Type
    - 5 b => Key length

    KEY is a set of bytes (whose length is indicated by the header) comprising the name of this entry
    > Whether this value is `remote` is stored in the high bit of the first character of the key

    VALUE is a set of bytes (whose length is indicated by the type) comprising the name of this entry
    > Note that for buffer types (str/buf), the length is stored as a `rval_bbuf_ptr_t` in the `VALUE` field, indicating the number of extra bytes required by the buffer
*/

// Supported data formats
typedef enum rval_type_t {
    _RVAL_I8         = 0b000,    // uint8_t
    _RVAL_I16        = 0b001,    // uint16_t
    _RVAL_I32        = 0b010,    // uint32_t
    _RVAL_FLOAT      = 0b011,    // float
    _RVAL_STR        = 0b100,    // char*
    _RVAL_STR_BYOB   = 0b101,    // char* (buffer provided)
    _RVAL_BUF        = 0b110,    // uint8_t*
    _RVAL_BUF_BYOB   = 0b111     // uint8_t* (buffer provided)
} rval_type_t;

// Internal error type
typedef enum rval_set_err_t {
    _RVAL_ERR_NONE,     // No error
    _RVAL_ERR_SAME,     // No change
    _RVAL_ERR_TOO_BIG,  // Buffer too big
    _RVAL_ERR_READONLY, // Remote value cannot be requested; Too big!
} rval_set_err_t;

// Used to view keys
// The same format as a string slice
typedef struct rval_key_view_t {
    const char* chars;    // The raw character data (non-NULL terminated)
    uint8_t length; // The number of characters stored in the `chars` buffer
} rval_key_view_t;

typedef struct rval_entry_header_t {
    uint8_t keyLen; // Length of key
    char start;     // Initial char of key
} rval_entry_header_t;

// Body type for buffers
typedef struct rval_entry_body_buf_t {
    rval_bbuf_ptr_t length;
    uint8_t start;
} rval_entry_body_buf_t;

typedef struct rval_buf_t {
    uint8_t* buf;           // The actual buffer data
    rval_bbuf_ptr_t cap;    // The size of the buffer

    // Dueal-headed pointers into `buf`
    rval_bbuf_ptr_t entry_len; // Number of bytes taken up by entries (grows from 0 to cap-1)
    rval_bbuf_ptr_t cache_idx; // Number of bytes taken up by cache (shrinks from cap-1 to 0)
} rval_buf_t;

typedef struct rval_t {
    cmd_t* cmd;     // The command instance we piggyback off of
    rval_buf_t buf; // The buffer to hold all internal data

    void (*onSet)(rval_key_view_t key); // Possibly null; Called on any value getting `set`
    bool (*onRes)(rval_key_view_t key); // Possibly null; Called on any value getting 'res'olved
} rval_t;

// Used to expose some buffer value to the outside world
typedef struct rval_buf_view_t {
    uint8_t* buf;
    rval_bbuf_ptr_t size;
} rval_buf_view_t;

/**
 * Create a new `rval` instance
 * @param cmd       The command instance to use for value synchronization
 * @param vbuf      The buffer used to hold onto vlaues
 * @param vbuf_size The size of the value buffer (in bytes)
 * @param onSet     The callback function called whenever any value is set. Set to NULL to omit.
 * @param onRes     The callback function called whenever any **local** value is requested to be
 * resolved by some remote.
 * Return `true` to run default behaviour (resolve with current value).
 * Return `false` to inhibit default behaviour (custom resolution implementation required)
 * Set to NULL to omit.
 * @returnsThe created command instance
 */
rval_t rval(
    cmd_t* cmd,
    uint8_t* vbuf,
    rval_bbuf_ptr_t vbuf_size,
    void (*onSet)(rval_key_view_t key),
    bool (*onRes)(rval_key_view_t key)
);

/**
 * Create a new `rval` instance quickly (fast)
 * Intended to be used as a on-size-fits-all drop-in solution to ongoing projects
 * Uses dynamic memory allocation to minimize boilerplate required in getting this running,
 * yielding a larger memory footprint (and the possiblity of failure) with the benefit of
 * an easier dev experience
 * If either the vbuf cannot be allocated, it is reduced to a size of 0 B
 * @param cmd       The command instance to use for value synchronization
 * @param vbuf_size The size of the value buffer (in bytes)
 * @param onSet     The callback function called whenever any value is set. Set to NULL to omit.
 * @param onRes     The callback function called whenever any **local** value is requested to be
 * resolved by some remote.
 * Return `true` to run default behaviour (resolve with current value).
 * Return `false` to inhibit default behaviour (custom resolution implementation required)
 * Set to NULL to omit.
 * @returnsThe created command instance
 */
rval_t rval_f(
    cmd_t* cmd,
    rval_bbuf_ptr_t vbuf_size,
    void (*onSet)(rval_key_view_t key),
    bool (*onRes)(rval_key_view_t key)
);

// ======== DECLARE VALUES ========

/**
 * Declare a new value with type `int8_t` within the `rval` system
 * @param rval      The `rval` instance to declare the value within
 * @param key       The key used to identify this value. Must be some non-empty NULL-terminated
 * ASCII cstring, otherwise this process will fail
 * @param remote    Whether this value is `remote`
 * If `true`: This value's source-of-truth is some other `rval` instance (set -> req)
 * If `false`: This `rval` instance is treated as the source-of-truth for the value (set -> set)
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Not enough memory to allocate the value/associated data
 * - Invalid input key (empty string)
 * - Repeated key (already exists)
 */
bool rval_decl_i8(rval_t* rval, const char* key, bool remote);

/**
 * Declare a new value with type `int16_t` within the `rval` system
 * @param rval      The `rval` instance to declare the value within
 * @param key       The key used to identify this value. Must be some non-empty NULL-terminated
 * ASCII cstring, otherwise this process will fail
 * @param remote    Whether this value is `remote`
 * If `true`: This value's source-of-truth is some other `rval` instance (set -> req)
 * If `false`: This `rval` instance is treated as the source-of-truth for the value (set -> set)
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Not enough memory to allocate the value/associated data
 * - Invalid input key (empty string)
 * - Repeated key (already exists)
 */
bool rval_decl_i16(rval_t* rval, const char* key, bool remote);

/**
 * Declare a new value with type `int32_t` within the `rval` system
 * @param rval      The `rval` instance to declare the value within
 * @param key       The key used to identify this value. Must be some non-empty NULL-terminated
 * ASCII cstring, otherwise this process will fail
 * @param remote    Whether this value is `remote`
 * If `true`: This value's source-of-truth is some other `rval` instance (set -> req)
 * If `false`: This `rval` instance is treated as the source-of-truth for the value (set -> set)
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Not enough memory to allocate the value/associated data
 * - Invalid input key (empty string)
 * - Repeated key (already exists)
 */
bool rval_decl_i32(rval_t* rval, const char* key, bool remote);

/**
 * Declare a new value with type `float` within the `rval` system
 * @param rval      The `rval` instance to declare the value within
 * @param key       The key used to identify this value. Must be some non-empty NULL-terminated
 * ASCII cstring, otherwise this process will fail
 * @param remote    Whether this value is `remote`
 * If `true`: This value's source-of-truth is some other `rval` instance (set -> req)
 * If `false`: This `rval` instance is treated as the source-of-truth for the value (set -> set)
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Not enough memory to allocate the value/associated data
 * - Invalid input key (empty string)
 * - Repeated key (already exists)
 */
bool rval_decl_f(rval_t* rval, const char* key, bool remote);

/**
 * Declare a new value with type `str` (char*) within the `rval` system
 * @param rval      The `rval` instance to declare the value within
 * @param key       The key used to identify this value. Must be some non-empty NULL-terminated
 * ASCII cstring, otherwise this process will fail
 * @param byob      The buffer used to store this string in. If `NULL`, the `vbuf` buffer is used
 * @param size      The length of the given buffer/size of buffer to allocate
 * @param remote    Whether this value is `remote`
 * If `true`: This value's source-of-truth is some other `rval` instance (set -> req)
 * If `false`: This `rval` instance is treated as the source-of-truth for the value (set -> set)
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Not enough memory to allocate the value/associated data
 * - Invalid input key (empty string)
 * - Repeated key (already exists)
 */
bool rval_decl_str(rval_t* rval, const char* key, char* byob, rval_bbuf_ptr_t size, bool remote);

/**
 * Declare a new value with type `buf` (uint8_t*) within the `rval` system
 * @param rval      The `rval` instance to declare the value within
 * @param key       The key used to identify this value. Must be some non-empty NULL-terminated
 * ASCII cstring, otherwise this process will fail
 * @param byob      The buffer used to store this buffer in. If `NULL`, this is stored directly in the `vbuf` buffer
 * @param size      The length of the given buffer/size of buffer to allocate
 * @param remote    Whether this value is `remote`
 * If `true`: This value's source-of-truth is some other `rval` instance (set -> req)
 * If `false`: This `rval` instance is treated as the source-of-truth for the value (set -> set)
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Not enough memory to allocate the value/associated data
 * - Invalid input key (empty string)
 * - Repeated key (already exists)
 */
bool rval_decl_buf(rval_t* rval, const char* key, uint8_t* byob, rval_bbuf_ptr_t size, bool remote);

/**
 * Undeclare some value and free up all memory associated with in in the internal buffer
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Not enough memory to allocate the value/associated data
 * - Invalid input key (doesn't already exist)
 */
bool rval_undecl(rval_t* rval, const char* key);

// ======== SET VALUES ========

/**
 * Update an integer value (i8, i16, i32) and notify remote if a change occurred
 * If this value is marked as `remote`, a `req` event will be triggered
 * Otherwise, a `set` event will be triggered
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @param value     The new value
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Invalid input key (doesn't match any declared values)
 * - Invlaid desired value (associated key is not an integer)
 */
bool rval_set_i(rval_t* rval, const char* key, int32_t value);

/**
 * Update a float value and notify remote if a change occurred
 * If this value is marked as `remote`, a `req` event will be triggered
 * Otherwise, a `set` event will be triggered
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @param value     The new value
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Invalid input key (doesn't match any declared values)
 * - Invlaid desired value (associated key is not a float)
 */
bool rval_set_f(rval_t* rval, const char* key, float value);

/**
 * Update a string value and notify remote if a change occurred
 * If this value is marked as `remote`, a `req` event will be triggered
 * Otherwise, a `set` event will be triggered
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @param value     The new NULL-terminated string value
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Invalid input key (doesn't match any declared values)
 * - Invlaid desired value (associated key is not a string)
 * - Invalid value (string too long to fit within buffer)
 */
bool rval_set_str(rval_t* rval, const char* key, const char* value);

/**
 * Update a buffer value (or a portion of the buffer) and notify remote if a change occurred
 * If this value is marked as `remote`, a `req` event will be triggered
 * Otherwise, a `set` event will be triggered
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @param value     The new buffer value
 * @param start     The byte offset within the buffer to start writing bytes from the new value buffer
 * Enables writing just a subset of the internal buffer
 * @param length    The number of bytes to copy from the new value buffer to the internal buffer
 * @returns`true` iff the operation succeeded. This may fail (and return `false`) for the following reasons:
 * - Invalid input key (doesn't match any declared values)
 * - Invlaid desired value (associated key is not a string)
 * - Invalid value (string too long to fit within buffer)
 */
bool rval_set_buf(rval_t* rval, const char* key, const uint8_t* value, rval_bbuf_ptr_t start, rval_bbuf_ptr_t length);

// ======== GET VALUES ========

/**
 * Get the current cached integer value
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @returns         The stored/cached integer value, or `0` if no integer entry with the given key exists
 */
uint32_t rval_get_i(rval_t* rval, const char* key);

/**
 * Get the current cached integer value
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @returns         The stored/cached float value, or `0` if no integer entry with the given key exists
 */
float rval_get_f(rval_t* rval, const char* key);

/**
 * Get the current cached string value
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @returns         The stored/cached string value, or NULL if no string entry with the given key exists
 * This pointer is not intended to be long-lived, and is only valid until the next `decl`/`undecl` call.
 */
const char* rval_get_str(rval_t* rval, const char* key);

/**
 * Get the current cached buffer value
 * @param rval      The `rval` instance to update the value within
 * @param key       The key used to identify this value
 * @returns         The stored/cached buffer value, or { NULL, 0 } if no buffer entry with the given key exists
 * This buffer view is not intended to be long-lived, and is only valid until the next `decl`/`undecl` call.
 */
rval_buf_view_t rval_get_buf(rval_t* rval, const char* key);

/**
 * Request the internal cache for some value be updated via a `resolve` packet
 * Note that this function is type agnostic
 * @param rval  The `rval` instance to resolve the value within
 * @param key   The key used to identify this value
 * @returns     `true` iff the operation succeeded
 * This may fail for the following reasons:
 * - Invalid `key` (value wasn't declared)
 * - The associated value is `remote` and the attached command instance is read-only
 */
bool rval_res(rval_t* rval, const char* key);

#ifdef __cplusplus
}
#endif

#endif