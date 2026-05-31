# rval

Remote value synchronization for embedded systems.

`rval` (Remote VALue) is an extension for `cmd.h` that provides lightweight synchronized state management between connected systems.

It lets multiple devices expose named values, update them, request refreshes, and keep local/remote state synchronized using the existing `cmd` transport layer.

Think of it as a tiny distributed variable system for embedded projects.

Typical use cases:

* synchronizing configuration between MCU nodes
* Remote parameter tuning
* Sensor value sharing
* Distributed robotics state
* Host/device control channels
* Runtime configuration interfaces
* Simple request/response state replication

---

# How It Works

Each value is declared as either:

## Local

This device owns the value. There should be exactly one owner of any given value within any network

When changed:

```txt
set -> broadcast update ("set")
```

Other connected systems update their cached copy.

---

## Remote

Another device owns the value. There may be any number of remotes within any network

When changed locally:

```txt
set -> request authoritative value ("req")
```

When written locally, a remote value issues a `req` packet instead of mutating cache immediately.

The cache is only updated after the authoritative owner responds with `set`.

---

## Protocol Commands

### `set`

Broadcast authoritative value update.

```text
set <key> <value>
```

---

### `req`

Request authoritative write/update.

```text
req <key> <value>
```

The owner may accept and rebroadcast the resulting value.

---

### `res`

Request retransmission of current authoritative state.

```text
res <key>
```

---

This allows systems to share ownership of synchronized state without duplicating application logic.

---

# Features

* Built on top of `cmd.h`
* Static allocation by default
* Optional dynamic allocation (`rval_f`)
* Integer, float, string, and binary buffer support
* Local/remote ownership semantics
* Change notifications
* Resolve requests
* Tiny memory footprint
* BYOB (bring-your-own-buffer) support for large buffer-style values
* Designed for serial/UART transport
- Easily human-operable/readable

---

# Supported Value Types

| Type    | Description           |
| ------- | --------------------- |
| `i8`    | 8-bit signed integer  |
| `i16`   | 16-bit signed integer |
| `i32`   | 32-bit signed integer |
| `float` | Floating-point value  |
| `str`   | String                |
| `buf`   | Binary buffer         |

String and buffer values support:

* Internal storage
* External BYOB storage

---

# Quick Start

## 1. Create a `cmd` Instance

```c
#include <Arduino.h>
#include "cmd.h"
#include "rval.h"

cmd_entry_t entries[8];
uint8_t cmd_buffer[128];

cmd_t shell;
```

---

## 2. Create an `rval` Instance

```c
rval_t rv;
uint8_t rval_buffer[128];
```

---

## 3. Initialize

```c
void setup() {

    Serial.begin(115200);

    shell = cmd(
        entries,
        8,
        cmd_buffer,
        sizeof(cmd_buffer),
        '!'
    );

    rval(
        &rv,
        &shell,
        rval_buffer,
        sizeof(rval_buffer),
        NULL,
        NULL
    );
}
```

---

## 4. Declare Values

```c
void setup() {

    ...

    rval_decl_i16(&rv, "speed", false);
    rval_decl_f(&rv, "temp", true);
    rval_decl_str(&rv, "mode", NULL, 16, false);
}
```

Meaning:

* `speed` → uint16_t, local
* `temp` → float, remote
* `mode` → char*, local

---

## 5. Use Normally

```c
void loop() {

    // Run `cmd.h` subsystem
    while (Serial.available()) {
        cmd_recv(&shell, Serial.read());
    }

    // Notify remotes of change
    rval_set_i(&rv, "speed", 1200);

    // Fetch locally cached value of remote
    float remoteTemp = rval_get_f(&rv, "temp");
}
```

---

# Full Arduino Example

```c
#include <Arduino.h>
#include "cmd.h"
#include "rval.h"

cmd_entry_t entries[8];
uint8_t cmd_buffer[128];

cmd_t shell;

rval_t rv;
uint8_t rval_buffer[128];

void setup() {

    Serial.begin(115200);

    shell = cmd(
        entries,
        8,
        cmd_buffer,
        sizeof(cmd_buffer),
        '!'
    );

    rval(
        &rv,
        &shell,
        rval_buffer,
        sizeof(rval_buffer),
        NULL,
        NULL
    );

    rval_decl_i16(&rv, "motor", false);
    rval_decl_f(&rv, "battery", true);
    rval_decl_str(&rv, "mode", NULL, 16, false);

    rval_set_i(&rv, "motor", 500);
    rval_set_str(&rv, "mode", "idle");
}

void loop() {

    while (Serial.available()) {
        cmd_recv(&shell, Serial.read());
    }

    uint32_t motor = rval_get_i(&rv, "motor");
    float battery = rval_get_f(&rv, "battery");

    Serial.print("Motor: ");
    Serial.println(motor);

    Serial.print("Battery: ");
    Serial.println(battery);

    delay(1000);
}
```

---

# Declaring Values

Values are stored in a key/value relationship, with string keys pointing to arbitrary-data values.

Keys should be restricted to simple printable ASCII identifiers.

Recommended:

motor.speed
battery
ui.mode

Avoid:

- quotes (`"`)
- escape characters
- control characters
- extended UTF-8

## Integer

```c
rval_decl_i8(&rv, "led", false);
rval_decl_i16(&rv, "rpm", false);
rval_decl_i32(&rv, "ticks", false);
```

---

## Float

```c
rval_decl_f(&rv, "temperature", true);
```

---

## String

Internal storage:

```c
rval_decl_str(&rv, "name", NULL, 32, false);
```

External buffer:

```c
char nameBuf[32];
rval_decl_str(&rv, "name", nameBuf, sizeof(nameBuf), false);
```

---

## Binary Buffer

> Buffer transport synchronization is not yet implemented.
>
> Buffer values can be declared, stored, mutated, and read locally,
> but are not currently serialized across cmd transport.

Internal storage:

```c
rval_decl_buf(&rv, "frame", NULL, 64, false);
```

External storage:

```c
uint8_t frameBuf[64];
rval_decl_buf(&rv, "frame", frameBuf, sizeof(frameBuf), false);
```

---

# Setting Values

## Integer (i8, i16, i32)

```c
rval_set_i(&rv, "rpm", 1400);
```

---

## Float

```c
rval_set_f(&rv, "temperature", 24.5f);
```

---

## String

```c
rval_set_str(&rv, "mode", "run");
```

---

## Buffer

Full write:

```c
uint8_t data[16];
rval_set_buf(&rv, "frame", data, 0, 16);
```

Partial write:

```c
rval_set_buf(&rv, "frame", patch, 8, 4);
```

---

# Reading Values

## Integer (i8, i16, i32)

```c
uint32_t rpm = rval_get_i(&rv, "rpm");
```

---

## Float

```c
float temp = rval_get_f(&rv, "temperature");
```

---

## String

```c
const char* mode = rval_get_str(&rv, "mode");
```

---

## Buffer

```c
rval_buf_view_t frame = rval_get_buf(&rv, "frame");
```

---

# Resolving Remote Values

To explicitly refresh a remote value:

```c
rval_res(&rv, "temperature");
```

This requests the authoritative source to resend the latest value.

Useful when:

* A device reconnects
* State may be stale
* synchronization is uncertain

---

# Callbacks

## onSet

Runs **only when cache actually changes**

Not when:

- value unchanged
- invalid type
- operation rejected

```c
bool onSet(rval_key_view_t key) {

    Serial.write(key.chars, key.length);
    Serial.println(" updated");

    // Value currently unused
    // Return true to garuntee compatibility with future versions
    return true;
}
```

---

## onRes

Runs only when:

- value is locally owned
- a remote peer sent `res`

Returning `false` suppresses automatic `set` response.


```c
bool onRes(rval_key_view_t key) {

    Serial.println("Resolve request");

    return true;
}
```

Returning:

* `true` → perform default resolve behavior
* `false` → custom response handling (inhibit default `set` response)

---

# Memory Model

Like `cmd`, `rval` uses a dual-headed buffer.

```txt
| entries --->       <--- cache |
+-------------------------------+
```

Entries store:

```txt
[ HEADER | KEY | VALUE ]
```

This minimizes memory usage, memory fragmentation, and avoids heap overhead.

---

# BYOB (Bring Your Own Buffer)

Large values can use externally-managed storage.

Useful for:

* Framebuffers
* Sensor capture buffers
* Packet storage
* Large strings

Example:

```c
uint8_t image[128];

rval_decl_buf(
    &rv,
    "image",
    image,
    sizeof(image),
    false
);
```

This prevents the internal value buffer from being taken over by buffers.
Note that the buffer value shall not be updated directly, and must adhere to the `rval.h` interface. Failure to follow this rule may cause remote values to become out-of-sync.

---

# Error Handling

Operations fail gracefully when:

* Buffer space is exhausted
* Duplicate keys are declared
* Invalid keys are used
* Type mismatches occur
* Values exceed storage capacity

All mutating operations return `bool` (true => success, false => failure).

Always check results when reliability matters.

---

# Typical Applications

## Remote Motor Tuning

```txt
motor.speed
motor.kp
motor.ki
motor.kd
```

---

## Sensor Networks

```txt
temperature
humidity
pressure
```

---

## UI synchronization

```txt
brightness
theme
menu.state
```

---

## Distributed Robotics

```txt
pose.x
pose.y
arm.position
battery
```

---

# Relationship to `cmd`

`rval` is transport-agnostic but depends on `cmd` for communication.

`cmd` handles:

* Parsing
* Dispatch
* framing

`rval` handles:

* State storage
* synchronization
* ownership
* resolution

Together they provide a lightweight distributed state system for embedded devices.

---

# Notes

* Built specifically for embedded/Arduino-style systems
* Not thread-safe
* String/buffer views may become invalid after any `decl`/`undecl` call. Never rely on returned pointers staying fresh
* Remote synchronization assumes a connected cooperating peer
* Internal pointer width is configurable via:
* Value lookups (used in both set/get) use binary search over a sorted internal cache, yielding determinstic O(log(n)) performance, providing effecient access even with many declared values

```c
#define rval_bbuf_ptr_t uint16_t
```

before including `rval.h`

# Limitations

`rval.h` was primarily designed for low-memory environments. This means that its memory footprint is intentionally low. Due to this goal, some comprimises have been made.

## Key Limits

To preserve space, the key is stored as a string slice (rather than as a cstring), with the slice length stored using 5 bits, yielding a maximum length of 31 characters.

Additionally, the bit denoting whether any given value in `rval.h` is "remote" is stored in the upper-most bit of the first character of the key, yielding a minimum length of 1 character, and the limit that _every_ value key must adhere to standard ASCII.

## rval_buf Limits

The rval buffer acts as a dual-headed allocator, used for both holding the key/value pairs and the cache pointing to these key/value pairs. Its size is stored, by default, as a `uint8_t`, limiting its maximum length to 256 entries.

This limit is in place to allow the cache entry pointers to the key/value pair to be small `uint8_t` bytes, rather than full addressing words.

If more memory is required, set the `rval_bbuf_ptr_t` macro to some larger type (ex: `uint16_t`). Note that this will increase caching overhead, so projects with a large amount of small values may suffer from a change like this.

> If you are unable to set this macro in your compiler (ex: using the Arduino IDE) but must have a greater cmd_buffer size, modify `rval.h:26` directly
