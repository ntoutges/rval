# cmd.h

Lightweight command parser for embedded systems and serial consoles.

`cmd.h` is a small single-header/single-source command parser intended primarily for UART-driven embedded projects. It parses commands incrementally, requires no dynamic allocation by default, and is designed to work well in interrupt-driven or byte-at-a-time receive loops.

Features include:

* Incremental character-by-character parsing
* Ordered arguments
* Named arguments (`--flag value`)
* Grouped short flags (`-abc`)
* Quoted strings (`"hello world"`)
* Zero-allocation operation
* Optional convenience allocator (`cmd_f`)
* Tiny API surface
* Callback-based command dispatch

---

# Command Format

Commands follow this format:

```txt
<initiator><command> <args>\n
```

Example:

```txt
!echo hello world
```

Where:

| Component   | Description                                     |
| ----------- | ----------------------------------------------- |
| `initiator` | Character indicating a command (`!`, `/`, etc.) |
| `command`   | Command name                                    |
| `args`      | Optional arguments                              |

---

# Supported Argument Types

## Ordered Arguments

Arguments without a leading `-` or `--` are treated as ordered arguments.

```txt
!flash firmware.bin verify
```

| Index | Value          |
| ----- | -------------- |
| `0`   | `flash`        |
| `1`   | `firmware.bin` |
| `2`   | `verify`       |

Access using:

```c
cmd_ogets(cmd, 1, "");
```

---

## Named Arguments

Named arguments use the format:

```txt
--name value
```

Example:

```txt
!uart --baud 115200 --port 2
```

Access using:

```c
int baud = cmd_ugeti(cmd, "baud", 9600);
int port = cmd_ugeti(cmd, "port", 0);
```

---

## Short Flags

Single-character flags can be grouped together.

```txt
!log -abc
```

Equivalent to:

```txt
!log -a -b -c
```

Access using:

```c
bool a = cmd_ugetb(cmd, "a", false);
bool b = cmd_ugetb(cmd, "b", false);
bool c = cmd_ugetb(cmd, "c", false);
```

---

## Quoted Strings

Quoted tokens preserve spaces.

```txt
!echo "Hello world"
```

Result:

| Index | Value         |
| ----- | ------------- |
| `0`   | `echo`        |
| `1`   | `Hello world` |

Quotes may be escaped using `\"`.

---

# Quick Start

## 1. Create a Command Instance

### Static Allocation (recommended)

```c
#include "cmd.h"

Cmd_entry_t entries[8];
uint8_t cmd_buffer[128];

cmd_t shell = cmd(
    '!',
    entries,
    8,
    cmd_buffer,
    sizeof(cmd_buffer),
    NULL
);
```

### Dynamic Allocation

```c
cmd_t shell = cmd_f('!' 8, 128, NULL);
```

---

## 2. Register Commands

```c
void cmd_ping(void* args) {
    Serial.println("pong");
}

void setup() {
    Serial.begin(115200);

    cmd_attach(&shell, "ping", cmd_ping, NULL);
}
```

---

## 3. Feed Incoming Characters

```c
void loop() {

    while (Serial.available()) {
        cmd_recv(&shell, Serial.read());
    }
}
```

Commands execute automatically when a newline (`\n`) is received.

---

# 4. Sending Commands

`cmd.h` can also transmit commands using a user-provided send callback.

This is useful for:

* Sending commands to another MCU
* Bridging commands across UART
* Creating command relays
* Building interactive command shells
* Testing command handlers programmatically

To enable transmission, provide a send callback when creating the command instance:

```c
void uart_send(char ch) {
    Serial.write(ch);
}

cmd_t shell = cmd(
    '!',
    entries,
    8,
    cmd_buffer,
    sizeof(cmd_buffer),
    uart_send
);
```

If `send` is set to `NULL`, the instance becomes **read-only** and transmit helpers will return `false`.

---

## Sending Individual Characters

Use `cmd_send()` to send a single character.

```c
cmd_send(&shell, 'p');
cmd_send(&shell, 'i');
cmd_send(&shell, 'n');
cmd_send(&shell, 'g');
cmd_send(&shell, '\n');
```

Passing `0x00` automatically sends the command initiator.

```c
cmd_send(&shell, 0x00);   // Sends '!'
cmd_send(&shell, 'p');
cmd_send(&shell, 'i');
cmd_send(&shell, 'n');
cmd_send(&shell, 'g');
cmd_send(&shell, '\n');
```

Equivalent output:

```txt
!ping
```

This is useful when constructing commands incrementally.

---

## Sending Full Strings

Use `cmd_sends()` to send an entire string.

```c
cmd_send(&shell, 0x00);
cmd_sends(&shell, "ping\n");
```

Or:

```c
cmd_send(&shell, 0x00);
cmd_sends(&shell, "led --brightness 128 -i fast\n");
```

Transmitted data:

```txt
!led --brightness 128 -i fast
```

**Important:** `cmd_sends()` does **not** automatically append `\n`.

If you want to execute the command immediately, include the newline yourself:

```c
cmd_send(&shell, 0x00);
cmd_sends(&shell, "ping\n");
```

---

## Arduino Example: Forwarding Commands

This makes it easy to bridge one serial interface to another.

```c
#include <Arduino.h>
#include "cmd.h"

void send_to_serial1(char ch) {
    Serial1.write(ch);
}

cmd_entry_t entries[8];
uint8_t cmd_buffer[128];

cmd_t shell;

void setup() {

    Serial.begin(115200);
    Serial1.begin(115200);

    shell = cmd(
        '!',
        entries,
        8,
        cmd_buffer,
        sizeof(cmd_buffer),
        send_to_serial1
    );
}

void loop() {

    if (Serial.available()) {
        cmd_send(&shell, Serial.read());
    }
}
```

This allows commands received on `Serial` to be forwarded directly to `Serial1`.

---

## Return Values

Both transmission helpers return `true` on success.

They return `false` if:

* `cmd` is `NULL` and no current command exists
* the command instance is read-only (`send == NULL`)

---

# The “Current Command” System

`cmd.h` keeps track of the most recently executed command instance internally.

This allows callbacks to use simplified getter functions:

```c
int baud = cmd_cugeti("baud", 9600);
```

instead of:

```c
int baud = cmd_ugeti(cmd, "baud", 9600);
```

This is mainly intended as a quality-of-life feature for callback handlers.

The current command instance is automatically updated whenever a command successfully executes.

You may also set it manually:

```c
cmd_curr(&shell);
```

---

# Memory Model

`cmd.h` is designed to minimize allocations and copying.

The receive buffer stores:

* raw token strings
* null terminators
* cache entries

inside the same shared buffer.

Cache entries grow backward from the end of the buffer while received characters grow forward from the start.

```txt
| characters --->       <--- cache |
+----------------------------------+
```

This allows parsing to occur with very little overhead.

---

# Buffer Sizing

The receive buffer must contain:

* incoming command text
* null terminators
* internal cache entries (2 B each)

Larger commands with many arguments require larger buffers.

Typical embedded usage works well with:

```c
uint8_t cmd_buffer[64];
```

or:

```c
uint8_t cmd_buffer[128];
```

depending on command complexity.

Note that this buffer is limited to a maximum size of 255 B.

---

# Error Handling

Malformed commands are discarded automatically.

Conditions that invalidate a command include:

* receive buffer overflow
* malformed flags
* incomplete argument structures

When an invalid command is detected, parsing enters a closed state and ignores input until the next newline.

Unknown commands are ignored silently.

---

# API Overview

## Initialization

```c
cmd_t cmd(...);
cmd_t cmd_f(...);
```

## Runtime

```c
bool cmd_recv(cmd_t* cmd, char ch);
bool cmd_recvs(cmd_t* cmd, const char* str);
void cmd_curr(cmd_t* cmd);
```

## Transmission

```c
bool cmd_send(cmd_t* cmd, char ch);
bool cmd_sends(cmd_t* cmd, const char* str);
```

## Command Registration

```c
uint8_t cmd_attach(...);
uint8_t cmd_detach(...);
```

## Ordered Argument Getters

```c
cmd_ogeti(...)
cmd_ogetf(...)
cmd_ogetb(...)
cmd_ogets(...)
```

## Named Argument Getters

```c
cmd_ugeti(...)
cmd_ugetf(...)
cmd_ugetb(...)
cmd_ugets(...)
```

## Convenience “Current” Variants

```c
cmd_cogeti(...)
cmd_cugeti(...)
...
```

---

# Full Example

```c
#include <Arduino.h>
#include "cmd.h"

cmd_entry_t entries[8];
uint8_t cmd_buffer[128];

cmd_t shell;

void cmd_led(void* args) {
    int brightness = cmd_cugeti("brightness", 255);

    bool invert = cmd_cugetb("i", false);

    const char* mode = cmd_cogets(1, "normal");

    Serial.print("brightness: ");
    Serial.println(brightness);

    Serial.print("invert: ");
    Serial.println(invert ? "yes" : "no");

    Serial.print("mode: ");
    Serial.println(mode);
}

void cmd_ping(void* args) {
    Serial.println("pong");
}

void setup() {
    Serial.begin(115200);

    shell = cmd(
        '!',
        entries,
        8,
        cmd_buffer,
        sizeof(cmd_buffer),
        NULL
    );

    cmd_attach(&shell, "led", cmd_led, NULL);
    cmd_attach(&shell, "ping", cmd_ping, NULL);

    Serial.println("Ready");
}

void loop() {
    while (Serial.available()) {
        cmd_recv(&shell, Serial.read());
    }
}
```

Example commands:

```txt
!ping
!led --brightness 128 -i fast
```

Example output:

```txt
pong
brightness: 128
invert: yes
mode: fast
```

---

# Notes

* Command matching is case-sensitive
* Commands are matched using exact string comparison
* `\r` characters are ignored automatically
* Commands execute only after receiving `\n`
* `--flag=value` syntax is not currently supported
* The library is not thread-safe
* Getter string pointers reference internal buffer memory and should not be retained long-term

---

# Typical Embedded Usage

`cmd.h` was primarily designed for:

* UART debug consoles
* shell interfaces
* USB CDC terminals
* CLI configuration systems
* small MCU projects
* Arduino-style serial command handling

Example:

```txt
!wifi --ssid "My Network" --pass "secret"
!motor --speed 1200 -rv
!save
!reboot
```

---

# Limitations

`cmd.h` was primarily designed for low-memory environments. This means that its memory footprint is intentionally low. Due to this goal, some comprimises have been made.

## cmd_buffer Limits

The command buffer acts as a dual-headed arena allocator, used for both holding the received command _and_ the cache entries used to interpret that command. Its size is stored, by default, as a `uint8_t`, limiting its maximum length to 256 entries.

This limit is in place to allow the cache entry pointers to the received command to be small `uint8_t` bytes, rather than full addressing words.

If more memory is required, set the `cmd_bbuf_ptr_t` macro to some larger type (ex: `uint16_t`). Note that this will increase caching overhead, so projects with a large amount of small flags may suffer from a change like this.

> If you are unable to set this macro in your compiler (ex: using the Arduino IDE) but must have a greater cmd_buffer size, modify `cmd.h:26` directly

## Command Limits

Keyed commands names (`--myKey`) must _not_ be greater than 253 characters in length (not including the `--` characters). Note that the values (`--ignoreMe myValue`) of in a keyed command have no such limitation, and can be as long as they want (provided they fit within the `cmd_buffer` limits)

## Entry Limits

THere may be up to 256 registered command entries (attached callback functions run whenever some specific command is received). Attempting to attach more callbacks will do nothing.
