#include "rval.h"

bool _rval_alloc(rval_t* rval, const char* key, rval_bbuf_ptr_t length, rval_bbuf_ptr_t* loc); // Attempt to allocate space for a new cache entry, entry header, and `length` bytes of memory in the internal heap. If unsuccessful, returns `false`; otherwise returns `true` and populates `loc` with the index of the cache
bool _rval_free(rval_t* rval, const char* key); // Free up all memory associated with the given key. If none exist, returns `false`. Otherwise returns `true`
bool _rval_search(rval_t* rval, const char* key, rval_bbuf_ptr_t* idx); // Search for the associated cache slot based on the given key. If none exist, returns `false` and populates `idx` with the insertion index; otherwise returns `true` and populates `idx` with the index

bool _rval_decl(rval_t* rval, const char* key, rval_type_t type, rval_bbuf_ptr_t length, bool remote, void** value); // Attempt to declare a new value of type `type`, taking up `length` bytes. If unsuccessful, returns `false`. Otherwise returns `true` and populates `value` a pointer to the allocated value memory

rval_set_err_t _rval_set(rval_t* rval, rval_bbuf_ptr_t idx, const void* newValue, rval_bbuf_ptr_t offset, rval_bbuf_ptr_t length, bool force); // Update some value

rval_entry_header_t* _rval_get_header(rval_t* rval, rval_bbuf_ptr_t idx); // Get the header information for some cache entry
void*  _rval_get_value(rval_t* rval, rval_bbuf_ptr_t idx); // Get the value information for some cache entry
void _rval_get_entry(rval_t* rval, rval_bbuf_ptr_t idx, rval_entry_header_t** header, void** value); // Get the header + value information for some cache entry

rval_type_t _rval_get_type(rval_t* rval, rval_bbuf_ptr_t idx); // Get a value's type
bool _rval_get_remote(rval_t* rval, rval_bbuf_ptr_t idx); // Get whether a value is remote

bool _rval_update_remote(rval_t* rval, rval_bbuf_ptr_t idx, const void* newValue); // Send out set/req update based on value

void _rval_on_update(rval_t* rval, bool isReq); // Helper to update values on set/req listener

void _rval_on_set(void* rval);
void _rval_on_req(void* rval);
void _rval_on_res(void* rval);

void rval(
    rval_t* rval,
    cmd_t* cmd,
    uint8_t* vbuf,
    rval_bbuf_ptr_t vbuf_size,
    bool (*onSet)(rval_key_view_t key),
    bool (*onRes)(rval_key_view_t key)
) {
    // Initialize `rval` instance
    *rval = (rval_t) {
        .cmd = cmd,
        .buf = (rval_buf_t) {
            .buf = vbuf,
            .cap = vbuf_size,
            .cache_idx = vbuf_size - sizeof(rval_bbuf_ptr_t),
            .entry_len = 0
        },
        .onSet = onSet,
        .onRes = onRes
    };

    // Attach set/res/req listeners
    cmd_attach(cmd, "set", _rval_on_set, rval);
    cmd_attach(cmd, "res", _rval_on_res, rval);
    cmd_attach(cmd, "req", _rval_on_req, rval);
}

bool rval_f(
    rval_t* rval_,
    cmd_t* cmd,
    rval_bbuf_ptr_t vbuf_size,
    bool (*onSet)(rval_key_view_t key),
    bool (*onRes)(rval_key_view_t key)
) {
    uint8_t* vbuf = (uint8_t*) malloc(vbuf_size * sizeof(uint8_t));
    if (vbuf == NULL) vbuf_size = 0; // Failed to allocate buffer

    rval(rval_, cmd, vbuf, vbuf_size, onSet, onRes);

    // Indicate if we were successful
    return vbuf != NULL;
}

bool rval_decl_i8(rval_t* rval, const char* key, bool remote) {
    uint8_t* value;
    
    // Allocate space for value
    if (!_rval_decl(rval, key, _RVAL_I8, sizeof(int8_t), remote, (void**) &value))
        return false;

    *value = 0; // Reset value to 0

    return true; // Success!
}

bool rval_decl_i16(rval_t* rval, const char* key, bool remote) {
    uint16_t* value;
    
    // Allocate space for value
    if (!_rval_decl(rval, key, _RVAL_I16, sizeof(int16_t), remote, (void**) &value))
        return false;

    *value = 0; // Reset value to 0

    return true; // Success!
}

bool rval_decl_i32(rval_t* rval, const char* key, bool remote) {
    uint32_t* value;
    
    // Allocate space for value
    if (!_rval_decl(rval, key, _RVAL_I32, sizeof(int32_t), remote, (void**) &value))
        return false;

    *value = 0; // Reset value to 0

    return true; // Success!
}

bool rval_decl_f(rval_t* rval, const char* key, bool remote) {
    float* value;
    
    // Allocate space for value
    if (!_rval_decl(rval, key, _RVAL_FLOAT, sizeof(float), remote, (void**) &value))
        return false;

    *value = 0; // Reset value to 0

    return true; // Success!
}

bool rval_decl_str(rval_t* rval, const char* key, char* byob, rval_bbuf_ptr_t size, bool remote) {
    if (size == 0) return false; // Cannot allocate string of size 0

    rval_entry_body_buf_t* value;

    // Determine type based on if `byob` buffer is provided
    rval_type_t type =       byob == NULL ? _RVAL_STR                       : _RVAL_STR_BYOB;
    rval_bbuf_ptr_t length = sizeof(rval_bbuf_ptr_t) + (byob == NULL ? size : sizeof(char*)); // Either the buffer itself or a pointer to a byob buffer

    // Allocate space for value
    if (!_rval_decl(rval, key, type, length, remote, (void**) &value)) return false;

    value->length = size; // Record allocated size of buffer

    if (byob == NULL) value->start = 0x00;              // Initialize as empty string iff our own buffer
    else {
        memcpy(&(value->start), &byob, sizeof(char*));  // Otherwise, copy in pointer to provided buffer
        byob[0] = 0x00;                                 // and initialize empty string
    }

    return true; // Success!
}

bool rval_decl_buf(rval_t* rval, const char* key, uint8_t* byob, rval_bbuf_ptr_t size, bool remote) {
    if (size == 0) return false; // Cannot allocate string of size 0

    rval_entry_body_buf_t* value;

    // Determine type based on if `byob` buffer is provided
    rval_type_t type =       byob == NULL ? _RVAL_BUF                       : _RVAL_BUF_BYOB;
    rval_bbuf_ptr_t length = byob == NULL ? sizeof(rval_bbuf_ptr_t) + size : sizeof(uint8_t*); // Either the buffer itself or a poitner to a byob buffer
    
    // Allocate space for value
    if (!_rval_decl(rval, key, type, length, remote, (void**) &value)) return false;

    value->length = size; // Record allocated size of buffer

    // Pointendly do NOT initialize buffer data
    // Only fill in buffer data to point at BYOB buffer
    if (byob != NULL) memcpy(&(value->start), &byob, sizeof(char*));

    return true; // Success!
}

bool rval_undecl(rval_t* rval, const char* key) {
    return _rval_free(rval, key);
}

bool rval_set_i(rval_t* rval, const char* key, int32_t value) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return false;

    rval_type_t type = _rval_get_type(rval, idx);
    
    // Invalid type; Fail!
    if (type != _RVAL_I8 && type != _RVAL_I16 && type != _RVAL_I32) return false;

    rval_set_err_t err = _rval_set(rval, idx, &value, 0, 0, false);
    return err == _RVAL_ERR_NONE || err == _RVAL_ERR_READONLY || err == _RVAL_ERR_SAME;
}

bool rval_set_f(rval_t* rval, const char* key, float value) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return false;

    rval_type_t type = _rval_get_type(rval, idx);
    
    // Invalid type; Fail!
    if (type != _RVAL_FLOAT) return false;

    rval_set_err_t err = _rval_set(rval, idx, &value, 0, 0, false);
    return err == _RVAL_ERR_NONE || err == _RVAL_ERR_READONLY || err == _RVAL_ERR_SAME;
}

bool rval_set_str(rval_t* rval, const char* key, const char* value) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return false;

    rval_type_t type = _rval_get_type(rval, idx);
    
    // Invalid type; Fail!
    if (type != _RVAL_STR && type != _RVAL_STR_BYOB) return false;

    rval_set_err_t err = _rval_set(rval, idx, value, 0, strlen(value), false);
    return err == _RVAL_ERR_NONE || err == _RVAL_ERR_READONLY || err == _RVAL_ERR_SAME;
}

bool rval_set_buf(rval_t* rval, const char* key, const uint8_t* value, rval_bbuf_ptr_t start, rval_bbuf_ptr_t length) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return false;

    rval_type_t type = _rval_get_type(rval, idx);
    
    // Invalid type; Fail!
    if (type != _RVAL_BUF && type != _RVAL_BUF_BYOB) return false;

    rval_set_err_t err = _rval_set(rval, idx, value, start, length, false);
    return err == _RVAL_ERR_NONE || err == _RVAL_ERR_READONLY || err == _RVAL_ERR_SAME;
}

uint32_t rval_get_i(rval_t* rval, const char* key) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return 0;

    rval_type_t type = _rval_get_type(rval, idx);
    
    // Invalid type; Fallback to 0
    if (type != _RVAL_I8 && type != _RVAL_I16 && type != _RVAL_I32) return 0;

    void* ival = _rval_get_value(rval, idx);

    // Extract relevant data given type and return
    switch (type) {
        case _RVAL_I8:
            return *((uint8_t*) ival);
        case _RVAL_I16:
            return *((uint16_t*) ival);
        case _RVAL_I32:
            return *((uint32_t*) ival);
        
        // How did you get here!?
        default:
            return 0;
    }

    // Seriously, how did you get here!?
    return 0;

}

float rval_get_f(rval_t* rval, const char* key) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return 0;

    rval_type_t type = _rval_get_type(rval, idx);
    
    // Invalid type; Fallback to 0
    if (type != _RVAL_FLOAT) return 0;

    return *((float*) _rval_get_value(rval, idx));
}

const char* rval_get_str(rval_t* rval, const char* key) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return NULL;

    rval_type_t type = _rval_get_type(rval, idx);
    
    // Invalid type; Fallback to NULL
    if (type != _RVAL_STR && type != _RVAL_STR_BYOB) return NULL;

    rval_entry_body_buf_t* ival = _rval_get_value(rval, idx);
    
    // Return pointer to buffer
    return type == _RVAL_STR
        ? (char*) &(ival->start)    // Internal buffer
        : *((char**) &ival->start); // BYOB (external) buffer
}

rval_buf_view_t rval_get_buf(rval_t* rval, const char* key) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return (rval_buf_view_t) { NULL, 0 };

    rval_type_t type = _rval_get_type(rval, idx);
    
    // Invalid type; Fallback to NULL
    if (type != _RVAL_BUF && type != _RVAL_BUF_BYOB) return (rval_buf_view_t) { NULL, 0 };

    rval_entry_body_buf_t* ival = _rval_get_value(rval, idx);
    
    return (rval_buf_view_t) {
        .buf = type == _RVAL_STR
            ? &(ival->start)                // Internal buffer
            : *((uint8_t**) &ival->start),  // BYOB (external) buffer
        .size = ival->length
    };
}

bool rval_res(rval_t* rval, const char* key) {
    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, key, &idx)) return false; // Invalid key

    if (!cmd_send(rval->cmd, 0x00))      // Initiate transmission
        return false; // Readonly command instance
    cmd_sends(rval->cmd, "res \""); // Resolving...
    cmd_sends(rval->cmd, key);      // Value to resolve
    cmd_sends(rval->cmd, "\"\n");   // Send transmission

    // Success!
    return true;
}

// ======== PRIVATE FUNCTIONS ========

bool _rval_search(rval_t* rval, const char* key, rval_bbuf_ptr_t* idx) {
    if (key[0] == 0x00) return false; // Invalid key length

    // Find upper/lower bounds to search (inclusive, exclusive)
    rval_bbuf_ptr_t lower = rval->buf.cache_idx + sizeof(rval_bbuf_ptr_t);
    rval_bbuf_ptr_t upper = rval->buf.cap;
    rval_bbuf_ptr_t mid = rval->buf.cache_idx;

    // Get length of original key
    uint8_t sKeyLen = strlen(key);

    // Perform binary search on bounds
    while (lower < upper) {
        mid = lower + (upper - lower) / (2 * sizeof(rval_bbuf_ptr_t)) * sizeof(rval_bbuf_ptr_t); // Pointer to entry

        // Extract header info from point
        rval_entry_header_t* header = _rval_get_header(rval, mid);

        uint8_t keyLen = header->keyLen & 0b00011111;

        const char* midStr = &(header->start);
        char midStr0 = midStr[0] & 0x7F;

        // Check first character manually (accounts for shenanigans around `remote` stored in bit 7)
        int cmp = (midStr0 != key[0])
            ? midStr0 < key[0] ? -1 : 1                 // Check first character manually to account for `remote` bit (bit 7)
            : strncmp(midStr + 1, key + 1, keyLen - 1); // Let strncmp handle all other characters

        // Different strings match up to minimum key length
        if (cmp == 0 && sKeyLen != keyLen) {
            cmp = sKeyLen > keyLen ? 1 : -1; // Longer string is "greater" (char > 0x00 terminator in cstrings)
        }
        
        // Midpoint too low; Move up
        if (cmp < 0) {
            lower = mid + sizeof(rval_bbuf_ptr_t);
            continue;
        }

        // Midpoint too high; Move down
        if (cmp > 0) {
            upper = mid;
            mid -= sizeof(rval_bbuf_ptr_t); // Adjust `mid` for proper insertion index on failure
            continue;
        }

        // Found `key`
        else {
            *idx = mid;
            return true;
        }
    }

    *idx = mid; // SHould be inserted here
    return false; // Failed to find index
}

bool _rval_alloc(rval_t* rval, const char* key, rval_bbuf_ptr_t length, rval_bbuf_ptr_t* loc) {
    uint8_t keyLen = strlen(key);

    // Preflight check: Ensure key isn't too long/short
    if (keyLen <= 0 || keyLen >= 32) return false; // Too long to fit into 5 bits/too short to store remote bit

    // Preflight check: Ensure enough memory is available for entry
    // Note that cache entry is already technically preallocated (don't need to check, as it is const size)
    //                          --- Header (u8)      (key) ---   - Body -
    rval_bbuf_ptr_t req_bytes = (sizeof(uint8_t) + keyLen + length);
    rval_bbuf_ptr_t avail_bytes = rval->buf.cache_idx < rval->buf.entry_len
        ? 0
        : rval->buf.cache_idx - rval->buf.entry_len;
    if (req_bytes > avail_bytes) return false; // Not enough memory available

    // Get insertion point of new cache entry (and check if entry already exists)
    rval_bbuf_ptr_t idx;
    bool exists = _rval_search(rval, key, &idx);
    if (exists) return false; // `key` aLready allocated; give up!

    // Make space for cache
    memmove(
        &(rval->buf.buf[rval->buf.cache_idx]),
        &(rval->buf.buf[rval->buf.cache_idx + sizeof(rval_bbuf_ptr_t)]),
        (idx - rval->buf.cache_idx) * sizeof(rval_bbuf_ptr_t)
    );
    *((rval_bbuf_ptr_t*) &(rval->buf.buf[idx])) = rval->buf.entry_len; // Populate cache entry

    // Populate entry header
    rval_entry_header_t* header = (rval_entry_header_t*) &(rval->buf.buf[rval->buf.entry_len]);
    header->keyLen = keyLen;
    memcpy(&(header->start), key, keyLen * sizeof(uint8_t));

    // Mark slots as taken
    rval->buf.cache_idx -= sizeof(rval_bbuf_ptr_t); // Consume 1 cache entry
    rval->buf.entry_len += req_bytes;               // Consume all required bytes

    // Update loc
    *loc = idx;

    return true;
}

bool _rval_free(rval_t* rval, const char* key) {

    // Search for cache entry
    rval_bbuf_ptr_t idx;
    bool exists = _rval_search(rval, key, &idx);
    if (!exists) return false; // `key` not allocated; no need to deallocate

    rval_entry_header_t* header;
    uint8_t* value;

    _rval_get_entry(rval, idx, &header, (void**) &value);

    rval_bbuf_ptr_t bidx = *((rval_bbuf_ptr_t*) &(rval->buf.buf[idx])); // Get base value idx
    uint8_t* base = &(rval->buf.buf[bidx]); // Get base of entry from idx

    // Extract important fields
    uint8_t type =  header->keyLen >> 5;
    uint8_t keyLen = header->keyLen & 0b00011111;

    rval_bbuf_ptr_t size = 0;

    // Get amount of space taken up by data
    switch (type) {
        case _RVAL_I8:      size = sizeof(int8_t);     break;
        case _RVAL_I16:     size = sizeof(int16_t);    break;
        case _RVAL_I32:     size = sizeof(int32_t);    break;
        case _RVAL_FLOAT:   size = sizeof(float);      break;
        
        case _RVAL_STR_BYOB:
        case _RVAL_BUF_BYOB:
            // Size entry + pointer to memory
            size = sizeof(rval_bbuf_ptr_t) + sizeof(char*);
            break;

        case _RVAL_STR:
        case _RVAL_BUF: {
            rval_entry_body_buf_t* val = (rval_entry_body_buf_t*) value; // Cast to more useful type

            // Size entry + that many bytes
            size = sizeof(rval_bbuf_ptr_t) + val->length;
            break;
        }
    }

    // Add amount of space taken up by header (1 B) + key
    size += 1 + keyLen;

    // Remove entry + cache
    // Allocater simplified by relying on a no-holes model, so we must fill in our gaps

    // Close entry hole by moving everyone ahead back
    // Allowed due to stable ids being keys, not pointers
    memmove(base, base + size, rval->buf.entry_len - size);

    // Close cache hole by moving everyone behind ahead by 1 cache entry
    memmove(
        rval->buf.buf + rval->buf.cache_idx + 2 * sizeof(rval_bbuf_ptr_t),
        rval->buf.buf + rval->buf.cache_idx + sizeof(rval_bbuf_ptr_t),
        idx - (rval->buf.cache_idx + sizeof(rval_bbuf_ptr_t))
    );

    // Update head pointers
    rval->buf.cache_idx += sizeof(rval_bbuf_ptr_t); // 1 less cache entry
    rval->buf.entry_len -= size;                    // `size` fewer bytes

    // Update all cache entries above `idx` to reflect new positions
    for (rval_bbuf_ptr_t i = rval->buf.cache_idx + sizeof(rval_bbuf_ptr_t); i < rval->buf.cap; i++) {
        rval_bbuf_ptr_t * const vidx = &(rval->buf.buf[i]);

        // Cache entry points to moved memory; Redirect to point at the real entries
        if (*vidx > bidx) *vidx -= size;
    }

    return true;
}

bool _rval_decl(rval_t* rval, const char* key, rval_type_t type, rval_bbuf_ptr_t length, bool remote, void** value) {
    rval_bbuf_ptr_t loc;

    bool success = _rval_alloc(rval, key, length, &loc);
    if (!success) return false; // Failed to allocate

    // Get entry data
    rval_entry_header_t* header;
    _rval_get_entry(rval, loc, &header, value);

    header->keyLen = (header->keyLen & ~0b11100000) | (type << 5);                      // Store type in high 3 bits of `keyLen` field
    header->start = (header->start & ~0b10000000) | (remote ? 0b10000000 : 0b00000000); // Store `remote` in high bit of `start` field

    return true;
}

rval_set_err_t _rval_set(rval_t* rval, rval_bbuf_ptr_t idx, const void* newValue, rval_bbuf_ptr_t offset, rval_bbuf_ptr_t length, bool force) {
    rval_type_t type = _rval_get_type(rval, idx);

    // Get old value for comparison
    rval_entry_header_t* header;
    void* oldValue;
    uint8_t* buf; // Buffer ptr able to be used to save work between comparison and application
    _rval_get_entry(rval, idx, &header, &oldValue);

    // Extract length of key from header field
    uint8_t keyLen = header->keyLen & 0b00011111;

    // Perform comparison check based on type
    switch (type) {
        case _RVAL_I8:
            if (*((int8_t*) oldValue) == *((int8_t*) newValue)) return _RVAL_ERR_SAME; // No difference
            break;
        case _RVAL_I16:
            if (*((int16_t*) oldValue) == *((int16_t*) newValue)) return _RVAL_ERR_SAME; // No difference
            break;
        case _RVAL_I32:
            if (*((int32_t*) oldValue) == *((int32_t*) newValue)) return _RVAL_ERR_SAME; // No difference
            break;
        case _RVAL_FLOAT:
            if (*((float*) oldValue) == *((float*) newValue)) return _RVAL_ERR_SAME; // No difference
            break;

        case _RVAL_STR:
        case _RVAL_STR_BYOB: {
            rval_entry_body_buf_t* parsedValue = (rval_entry_body_buf_t*) oldValue; // Extract relevant fields
            
            buf = type == _RVAL_STR
                ? &(parsedValue->start)                 // Buffer is inline
                : *((uint8_t**) &parsedValue->start);   // Buffer pointing somewhere else

            if (offset + length + 1 > parsedValue->length) return _RVAL_ERR_TOO_BIG; // Unable to hold desired string
            if (strncmp(((char*) buf) + offset, (char*) newValue, parsedValue->length - offset) == 0) return _RVAL_ERR_SAME; // No difference
            break;
        }

        case _RVAL_BUF:
        case _RVAL_BUF_BYOB: {
            rval_entry_body_buf_t* parsedValue = (rval_entry_body_buf_t*) oldValue; // Extract relevant fields
            
            buf = type == _RVAL_BUF
                ? &(parsedValue->start)                 // Buffer is inline
                : *((uint8_t**) &parsedValue->start);   // Buffer pointing somewhere else

            if (offset + length > parsedValue->length) return _RVAL_ERR_TOO_BIG; // Unable to hold desired buffer
            if (memcmp(buf + offset, (char*) newValue, parsedValue->length - offset) == 0) return _RVAL_ERR_SAME; // No difference
            break;
        }
    }

    bool remote = _rval_get_remote(rval, idx);

    // Apply new values
    // Separated from comparison to allow for cleanr set vs. req code
    // Assume that all save operations are now valid
    if (!remote || force) {
        switch (type) {
            case _RVAL_I8:
                memcpy(oldValue, newValue, sizeof(int8_t));
                break;
            case _RVAL_I16:
                memcpy(oldValue, newValue, sizeof(int16_t));
                break;
            case _RVAL_I32:
                memcpy(oldValue, newValue, sizeof(int32_t));
                break;
            case _RVAL_FLOAT:
                memcpy(oldValue, newValue, sizeof(float));
                break;

            case _RVAL_STR:
            case _RVAL_STR_BYOB:
                memcpy(buf + offset, newValue, length);
                buf[offset + length] = 0x00; // Push in NULL byte
                break;

            case _RVAL_BUF:
            case _RVAL_BUF_BYOB:
                memcpy(buf + offset, newValue, length);
                break;
        }

        // Call `onSet` if available
        if (rval->onSet != NULL) {
            rval->onSet((rval_key_view_t) {
                .chars = &header->start,
                .length = keyLen
            });
        }
    }

    // (remote && force) => received `set` from local source
    // Don't need to send out another `req` packet once acknowledged
    if (!(remote && force)) {
        if (!_rval_update_remote(rval, idx, newValue)) {
            return _RVAL_ERR_READONLY;
        }
    }

    return _RVAL_ERR_NONE;
}

bool _rval_update_remote(
    rval_t* rval,
    rval_bbuf_ptr_t idx,
    const void* newValue
) {
    // Extract relevant data
    rval_entry_header_t* header;
    const rval_entry_body_buf_t* value;

    _rval_get_entry(rval, idx, &header, (void**) &value);

    // Apply default value
    if (newValue != NULL) value = newValue;

    uint8_t keyLen   = header->keyLen & 0b00011111; // Separate keyLen from type
    rval_type_t type = header->keyLen >> 5;         // Separate type from keyLen
    bool remote = header->start & 0b10000000;       // Extract `remote` from upper bit of first key character

    const uint8_t* buf;

    if (
        type == _RVAL_STR
        || type == _RVAL_STR_BYOB
        || type == _RVAL_BUF
        || type == _RVAL_BUF_BYOB
    ) {
        if (newValue != NULL) {
            // Copy new value directly into buffer for buffer types
            buf = newValue;
        }
        else {
            // Extract buffer for buffer types
            buf = type == _RVAL_STR_BYOB || type == _RVAL_BUF_BYOB
                ? *((uint8_t**) &value->start)
                : &(value->start);
        }
    }

    // Value change verified; Initiate new request
    bool writable = cmd_send(rval->cmd, 0x00); // Send initiator
    if (!writable) return false; // No writer function available

    // Send initial command
    cmd_sends(
        rval->cmd,
        remote ? "req \"" : "set \""
    );
    
    // Send key
    for (uint8_t i = 0; i < keyLen; i++) {
        char ch = (&(header->start))[i];

        if (i == 0) ch &= 0x7F; // Clear high bit

        if (ch == '"') cmd_send(rval->cmd, '\\'); // Escape quotes
        cmd_send(rval->cmd, ch);
    }

    cmd_sends(rval->cmd, "\" ");

    // Send value
    switch (type) {
        case _RVAL_I8:
        case _RVAL_I16:
        case _RVAL_I32: {

            // Extract int from `newValue`
            int32_t val = type == _RVAL_I8
                ? *((int8_t*) value)
                : type == _RVAL_I16
                ? *((int16_t*) value)
                : *((int32_t*) value);
            
            cmd_sendi(rval->cmd, val);
            break;
        }

        case _RVAL_FLOAT: {
            // Extract float from `newValue`
            float val = *((float*) value);

            cmd_sendf(rval->cmd, val);
            break;
        }

        // Send string surrounded by quotes
        case _RVAL_STR:
        case _RVAL_STR_BYOB:
            cmd_send(rval->cmd, '\"');
            cmd_sendr(rval->cmd, (char*) buf);
            cmd_send(rval->cmd, '\"');
            break;

        case _RVAL_BUF:
        case _RVAL_BUF_BYOB:
            // @TODO
            break;
    }

    // Finish transmission
    cmd_send(rval->cmd, '\n');

    // Success!
    return true;
}

rval_entry_header_t* _rval_get_header(rval_t* rval, rval_bbuf_ptr_t idx) {
    rval_bbuf_ptr_t* addr = (rval_bbuf_ptr_t*) &(rval->buf.buf[idx]); // Get address from cache idx
    return (rval_entry_header_t*) &(rval->buf.buf[*addr]);            // Compute real pointer address from internal minified cache addr
}

void* _rval_get_value(rval_t* rval, rval_bbuf_ptr_t idx) {
    rval_entry_header_t* header;                            // Placeholder
    uint8_t* value;                                         // Value to be returned
    _rval_get_entry(rval, idx, &header, (void*) &value);    // Get value

    return value;
}

void _rval_get_entry(rval_t* rval, rval_bbuf_ptr_t idx, rval_entry_header_t** header, void** value) {
    *header = _rval_get_header(rval, idx);  // Copy over header data
    uint8_t keyLen = (*header)->keyLen & 0b00011111;

    *value = (void*) (((uint8_t*) *header) + sizeof(uint8_t) + keyLen);   // Data immediately after header (keyLen + key)
}

rval_type_t _rval_get_type(rval_t* rval, rval_bbuf_ptr_t idx) {
    rval_entry_header_t* header = _rval_get_header(rval, idx);

    // Extract `type` from keyLen field
    return header->keyLen >> 5;
}

bool _rval_get_remote(rval_t* rval, rval_bbuf_ptr_t idx) {
    rval_entry_header_t* header = _rval_get_header(rval, idx);

    // Extract `type` from keyLen field
    return (header->start & 0b10000000) ? true : false;
}

// ======== COMMAND LISTENERS ========

void _rval_on_update(rval_t* rval, bool isReq) {

    // Expected format: !set/req <name> <value> [--offset <offset>]
    const char* name = cmd_ogets(rval->cmd, 1, "");

    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, name, &idx)) return; // Invalid name; Ignore!

    // Extract referenced value type
    rval_type_t type = _rval_get_type(rval, idx);

    // `req` only responds if we own the value (!remote)
    // `set` only responds if we don't own the value (remote)
    if (isReq == _rval_get_remote(rval, idx)) return;

    switch (type) {
        
        // Treat all integers the same
        case _RVAL_I8:
        case _RVAL_I16:
        case _RVAL_I32: {
            int32_t value = cmd_ogeti(rval->cmd, 2, 0);
            _rval_set(rval, idx, &value, 0, 0, true);
            break;
        }

        // Extract float
        case _RVAL_FLOAT: {
            float value = cmd_ogetf(rval->cmd, 2, 0);
            _rval_set(rval, idx, &value, 0, 0, true);
            break;
        }

        case _RVAL_STR:
        case _RVAL_STR_BYOB: {
            const char* value = cmd_ogets(rval->cmd, 2, "");
            rval_bbuf_ptr_t offset = cmd_ugeti(rval->cmd, "offset", 0);
            _rval_set(rval, idx, value, offset, strlen(value), true);
        }

        case _RVAL_BUF:
        case _RVAL_BUF_BYOB:
            // @TODO handleme
            break;
    }
}

void _rval_on_set(void* args) {
    rval_t* rval = args;
    _rval_on_update(rval, false);
}

void _rval_on_req(void* args) {
    rval_t* rval = args;
    _rval_on_update((rval_t*) rval, true);
}

void _rval_on_res(void* args) {
    rval_t* rval = args;

    // Expected format: !res <name>
    const char* name = cmd_ogets(rval->cmd, 1, "");

    rval_bbuf_ptr_t idx;
    if (!_rval_search(rval, name, &idx)) return;    // Invalid name; Ignore!
    if (_rval_get_remote(rval, idx)) return;        // Remote value; Ignore (defer to local)

    // Notify listener (if available)
    if (rval->onRes) {
        bool inhibit = !rval->onRes((rval_key_view_t) {
            .chars = name,
            .length = strlen(name)
        });

        // Ignore default behaviour on inhibit (return false)
        if (inhibit) {
            return;
        }
    }

    _rval_update_remote(rval, idx, NULL); // NULL => Pull current value
}

int8_t rval_strcmp(rval_key_view_t lhs, const char* rhs) {

    char l, r;
    uint8_t i;
    for (i = 0; i < lhs.length; i++) {
        l = lhs.chars[i];
        r = rhs[i];
        
        if (l < r) return -1; // LHS appears **before** RHS, lexicographically
        else if (l > r) return 1; // LHS appears **after** RHS, lexicographically
    }

    if (rhs[i] != 0x00) return -1; // RHS is longer => LHS appears **before** RHS, lexicographically

    return 0; // Strings are equal!
}
