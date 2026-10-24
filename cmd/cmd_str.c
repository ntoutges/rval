#include "cmd_str.h"
#include <stdio.h>

int _cmd_str_itoa(int32_t v, bool padStart, char* buf, int size); // Convert some unsigned int to a string; Returns the number of digits used on success, or a negative value indicating the number of digits required on failure. Does _NOT_ add NULL terminator

char* _cmd_str_ftoma_inf(char* buf, int size); // Populate field with 'infinity'
int _cmd_str_ftoma_mantissa(double v, int16_t shift, char* buf, int size, bool* reqCarry); // Generate the decimal expansion of the mantissa; Returns the number of digits used for the mantissa. Does _NOT_ add NULL terminator
int _cmd_str_ftoma_exponent(int16_t e, char* buf, int8_t size); // Generate the decimal expansion of the exponent (including "e"). Returns the number of digits used for the mantissa
int _cmd_str_ftoma_full(double val, int16_t shift, char* buf, int size); // Generate full floating point decimmmal expansiumber of digits used for the mantissa
uint8_t _cmd_str_ftoma_dec_digits(int16_t x);

char* cmd_str_ftoma(double val, char* buf, int size) {
    char* start = buf;
    
    // Invalid sizes (not enough space to do anything)
    if (size == 0) return buf;
    if (size < 2) {
        buf[0] = 0x00;
        return buf;
    }

    // Ensure value is a number
    if (isnan(val)) {
        if (size >= 4) strcpy(buf, "NaN");  // NaN value
        else strcpy(buf, "N");              // Not enough space for full NaN; Truncate
        return buf;
    }

    // Zero case
    if (val == 0) {
        strcpy(buf, "0");
        return start;
    }

    // Immediately handle negatives
    // Beyond this condition, `val` is garunteed to be >= 0
    if (val < 0) {

        // Append '+-' to buffer
        buf[0] = '+';

        // Out of space in buffer
        // Push NULL byte then give up
        if (size <= 3) {
            buf[1] = 0x00;
            return start;
        }

        buf[1] = '-';
        buf = buf + 2;
        size -= 2;
        val = -val;
    }

    // Ensure value isn't infinite
    if (isinf(val)) {
        _cmd_str_ftoma_inf(buf, size);
        return start;
    }

    // Use union to perform evil bit hacks
    union {
        double f;
        uint64_t u;
    } v = { .f = val };

    // Extract mantissa and exponent fields
    // int m = 0x00800000 | (v.u & 0x007FFFFF);
    int e = ((v.u >> 52) & 0x7FF) - 1023;

    // (e * log10(2)) to get parameter estimating # of leading zeroes
    int16_t k = (((int) e * 1233) >> 12);

    // Refine digit estimate to exact value
    const double EPSILON = 1e-8;
    if (val > pow(10, k+1) - EPSILON) k++;
    else if (val < pow(10, k)) k--;

    int16_t shift = 0; // Default: Print out plain number

    // Determine proper value for `shift` based on size
    if (k <= -4) shift = k;
    if (k >= size - 1) shift = k;

    int len = _cmd_str_ftoma_full(val, shift, buf, size);
    
    // Something went wrong...
    if (len < 0) {
        buf[0] = 0x00;
    }

    return start;
}

char* cmd_str_itoa(int32_t val, char* buf, int size) {

    // Invalid sizes (not enough space to do anything)
    if (size == 0) return buf;
    if (size < 2) {
        buf[0] = 0x00;
        return buf;
    }

    char* obuf = buf;

    if (val < 0) {

        // Add numeric indicator if required
        buf[0] = '+';
        buf = &(buf[1]);
        size--;

        // Invalid size (not enough space!)
        if (size < 2) {
            buf[0] = 0x00;
            return obuf;
        }
    }

    int len = _cmd_str_itoa(val, false, buf, size - 1);
    
    // Unable to fit string in space
    // Return empty string
    if (len < 0) buf[0] = 0x00;

    // Add NULL terminator
    else buf[len] = 0x00;

    return obuf;
}

double cmd_str_atof(char* buf) {
    if (buf == NULL) return 0; // Invalid buffer; Parse as 0

    bool neg = false;
    float val = 0;

    // Advance buffer if first character is numeric indicator
    if (buf[0] == '+') {
        buf = &(buf[1]);
    }

    // Account for negative numbers
    if (buf[0] == '-') {
        neg = true;
        buf = &(buf[1]);
    }

    // Check for Inf
    if (buf[0] == 'I') {
        if (strcmp(buf, "I") == 0 || strcmp(buf, "Inf") == 0 || strcmp(buf, "Infinity") == 0)
            val = INFINITY;
        else val = 0; // Invalid number
    }

    // Check for NaN
    else if (buf[0] == 'N') {
        if (strcmp(buf, "N") == 0 || strcmp(buf, "NaN") == 0)
            val = NAN;
        else val = 0; // Invalid number
    }

    // Parse using atof
    else {
        val = atof(buf);
    }

    return neg ? -val : val;
}

int32_t cmd_str_atoi(char* buf) {
    if (buf == NULL) return 0; // Invalid buffer; Parse as 0

    // Advance buffer if first character is numeric indicator
    if (buf[0] == '+') {
        buf = &(buf[1]);
    }

    // No special cases to handle
    return atoi(buf);
}

// ======== PRIVATE FUNCTIONS ========

char* _cmd_str_ftoma_inf(char* buf, int size) {
    if (size >= 9) strcpy(buf, "Infinity"); // Infinity value
    else if (size >= 4) strcpy(buf, "Inf"); // Shorter infinity
    else if (size >= 2) strcpy(buf, "I");   // Truncated infinity
    else buf[0] = 0x00;                     // No space left...
    return buf;
}

int _cmd_str_itoa(int32_t v, bool padStart, char* buf, int size) {
    int len = 0;

    // Handle negative numbers
    // Don't negify here to allow _all_ negative values (including extreme) to work
    if (v < 0) buf[len++] = '-';
    
    // Generate string in reverse order
    do {
        
        // Account for negative values
        int8_t d = (v % 10);
        if (d < 0) d = -d;

        if (len < size) buf[len++] = d + '0';
        v = v / 10;

        if (v < 0) v = -v;
    } while (v > 0);

    // Too many digits; Indicate failure
    if (len > size) return -len;

    if (padStart) {
        while (len < size) buf[len++] = '0';
    }

    // Reverse string
    char tmp;
    char* src = &(buf[ buf[0] == '-' ? 1 : 0 ]);
    char* dst = &(buf[len - 1]);
    while (src < dst) {
        
        // Swap src/dst
        tmp = *src;
        *src = *dst;
        *dst = tmp;

        // Advance src/dst pointers
        src++;
        dst--;
    }

    return len;
}

int _cmd_str_ftoma_mantissa(double v, int16_t shift, char* buf, int size, bool* reqCarry) {
    // Adjust `v` to account for `shift`
    v *= pow(10, -shift);

    int32_t ipart = (int32_t) v; // Extract whole component
    double fpart = v - ipart;     // Extract decimal component

    int len = _cmd_str_itoa(ipart, false, buf, size);
    if (len < 0) return len;

    int roundFrom = fpart > 0.5 ? len - 1 : -1;

    // No space/reasoning for '.' or trailing doubleing point components
    // Ignore and stop with ipart mantissa
    int precision = size - len - 1; // Number of digits of precision available
    if (precision > 0 && fpart != 0) {
        buf[len++] = '.';

        // Print `precision` digits in chunks of 9 digits (to avoid integer overflow)
        while (precision > 0) {
            uint8_t subprecision = precision > 9 ? 9 : precision;
            precision -= subprecision;

            // No more precision to add
            if (fpart == 0) {
                break;
            }

            // Extract fpart/ipart
            fpart *= pow(10, subprecision);
            int32_t ipart = (int32_t) fpart;
            fpart -= ipart;

            // Assume success
            len += _cmd_str_itoa(ipart, true, buf + len, subprecision);
        }

        roundFrom = fpart > 0.5 ? len - 1 : -1;

        // Round up if necessary
        if (roundFrom != -1) {
            while (roundFrom-- >= 0) {
                int i = roundFrom + 1;
                if (buf[i] == '.') break; // Stop at decimal point
                
                // Carry over to next digit
                if (buf[i] == '9') {
                    buf[i] = '0';
                    continue;
                }

                // Finished carrying!
                buf[i]++;
                roundFrom = -1;
                break;
            }
        }

        // Remove trailing zeroes
        while (buf[len - 1] == '0') len--;

        // Remove trailing "."
        if (buf[len-1] == '.') len--;
    }

    // Round up if necessary
    if (roundFrom != -1) {
        while (roundFrom-- >= 0) {
            int i = roundFrom + 1;

            // Carry over to next digit
            if (buf[i] == '9') {
                buf[i] = '0';
                continue;
            }

            // Finished carrying!
            buf[i]++;
            roundFrom = -1;
            break;
        }

        if (roundFrom != -1) {
            int moveCt = len == size ? len - 1 : len;

            memmove(buf + 1, buf, moveCt); // Shift over digits
            buf[0] = '1';                  // Place  '1' into first digit

            *reqCarry = moveCt != len;     // Notify parent that we need to increment exponent
            len += *reqCarry ? 0 : 1;      // Request extra digit, if possible
        }
    }

    return len;
}

int _cmd_str_ftoma_exponent(int16_t e, char* buf, int8_t size) {
    uint8_t len = 0;

    // Add exponent marker
    buf[len++] = 'E';

    // Add negative sign to exponent
    if (e < 0) {
        buf[len++] = '-';
        e = -e;
    }

    int exp_len = _cmd_str_itoa(e, false, &(buf[len]), size - len);
    if (exp_len < 0) return exp_len - len;
    return exp_len + len;
}

uint8_t _cmd_str_ftoma_dec_digits(int16_t x) {
    uint16_t ax = x >= 0 ? x : -x;

    return (x < 0 ? 1 : 0)                                      // Account for negative sign
        + (ax >= 1000 ? 4 : ax >= 100 ? 3 : ax >= 10 ? 2 : 1);  // floor(log10(k + 1))
}

int _cmd_str_ftoma_full(double val, int16_t shift, char* buf, int size) {
    uint8_t shift_len;

    // Extract length of decimal expansion of shift
    if (shift == 0) shift_len = 0;  // No exponent; Collapses to the empty string
    else {
        // Get length of `shift` in decimal expansion
        shift_len = 1 + _cmd_str_ftoma_dec_digits(shift);
    }

    int mantissa_len = size - shift_len - 1;

    // Not enough space for mantissa
    if (mantissa_len <= 0) {
        return strlen(_cmd_str_ftoma_inf(buf, size));
    }

    // Start with mantissa
    bool reqCarry = false;
    int len = _cmd_str_ftoma_mantissa(val, shift, buf, mantissa_len, &reqCarry);
    if (len < 0) return len; // Failed to fit mantissa

    if (reqCarry) {
        shift++;
        uint8_t new_len = 1 + _cmd_str_ftoma_dec_digits(shift);

        // Need extra digit for exponent
        // Steal from mantissa
        if (new_len != shift_len) {
            shift_len = new_len;

            if (mantissa_len > 1) {
                mantissa_len--;
                len--;
            }
            
            // Unable to fit rounded value within allocated space
            // INF!
            else {
                return strlen(_cmd_str_ftoma_inf(buf, size));
            }
        }
    }

    // No exponent (only mantissa); Done!
    if (shift == 0) {
        buf[len++] = 0x00; // Add terminating byte
        return len;
    }

    // Fill in exponent
    int exp_len = _cmd_str_ftoma_exponent(shift, &(buf[len]), shift_len);
    if (exp_len < 0) return exp_len - len;
    len += exp_len;

    buf[len++] = 0x00; // Add terminating byte
    return len;
}
