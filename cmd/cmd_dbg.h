#ifndef _CMD_DBG_H
#define _CMD_DBG_H

#include <stdio.h>
#include <string.h>
#include "cmd.h"

/**
 * @file cmd_dbg.h
 * @author Nicholas T.
 * @brief Debugging and diagnostic tools for `cmd.h
 * @version 0.1
 * @date 2026-05-23
 * 
 * @copyright PiCO 2026
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Clear the state of the `cmd` object
 * @param cmd   The `cmd` instance to debug
 */
void cmd_dbg_clr(cmd_t* cmd);

/**
 * Print out the state of the internal buf_buf arena
 * Note that this will _NOT_ attempt to parse `chr_len` or `cch_idx`, to reduce
 * the number of internal variables that must be valid
 * @param cmd   The `cmd` instance to debug
 */
void cmd_dbg_bbuf(cmd_t* cmd);


#ifdef __cplusplus
}
#endif

#endif