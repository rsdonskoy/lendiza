# lendiza
A fast x86/x86_64 length disassembler — usable from user space and kernel.

### Which header do I include?
| context | header | needs |
|---|---|---|
| user-space C++ | `lendiza.hpp` | C++20 (`std::span`) |
| user-space C / other languages | `lendiza_export.h` + `liblendiza` | none |
| Windows driver (`cl /kernel`) | `lendiza_core.hpp` | C++17, WDK headers |
| Linux kernel module | `lendiza_core.hpp` | C++17, `-nostdinc++` |
| any kernel: types & error codes only | `lendiza_kernel.h` | C or C++ |

The core is header-only, has no STL, no exceptions, no RTTI, no virtual dispatch
and no mutable global state, so it is safe at `DISPATCH_LEVEL` or from interrupt
context. The static library only carries the `extern "C"` wrappers; **never link
a user-built `.lib`/`.a` into a driver** — compile the headers with the module.

### Build (Ninja recommended)
```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build -L quick          # suites
ctest --test-dir build -L kernel-safety  # guard-page + ASan probes
```
Produces two static libraries (`lendiza`, `lendiza_d`), the example executables
and the test binaries; `cmake --install build --prefix <dir>` installs libraries,
headers and CMake package files, so downstream projects can use
`find_package(lendiza CONFIG REQUIRED)`.

### C++ usage
```c++
#include "lendiza.hpp"

constexpr std::array<uint8_t, /* N */> code = {
    0x41, 0x56, 0x56, 0x57, 0x53, 0x48, 0x83, 0xEC, 0x68, 0x48, 0x89, 0xCE, /* ... */
};
constexpr std::span<const uint8_t> buffer{ code };
size_t len{ 0 };
for (auto itor = buffer.begin(); itor < buffer.end(); itor += len) {
    const std::span<const uint8_t> remaining{ itor, static_cast<std::size_t>(buffer.size() - (itor - buffer.begin())) };
    len = lendiza::ldiza_x86<64>()(remaining);
}
```

### C usage
```c
#include "lendiza_export.h"
#include <stdio.h>

int main(void) {
    const uint8_t code[] = { 0x90, 0x90, 0xC3 };
    size_t offset = 0;
    while (offset < sizeof(code)) {
        const size_t len = lendiza_disasm_x64(code + offset, sizeof(code) - offset);
        if (len >= LENDIZA_ERR_INSUFFICIENT_BUFFER) {
            fprintf(stderr, "decode error: %zu\n", len);
            return 1;
        }
        offset += len;
    }
    return 0;
}
```

### Kernel usage
```c++
/* Windows driver or Linux module: same two lines, plus the platform's headers. */
#include "lendiza_core.hpp"

uint8_t window[LDZ_MAX_INSNS_LEN];        /* 15: longest instruction, prefixes included */
copy_in(bytes, window);                   /* copy_from_user / MmCopyMemory - see below */

const size_t len = lendiza::disasm_x64(window, sizeof(window));
if (lendiza::is_error(len)) { /* 0xE0 not enough bytes, 0xE1 undefined, 0xE2 internal */ }
```

Two rules make this safe:

1. **Hand over at most `LDZ_MAX_INSNS_LEN` (15) readable bytes.** If fewer remain
   in the buffer, pass what you have: a truncated encoding returns
   `LENDIZA_ERR_INSUFFICIENT_BUFFER` rather than a guessed length, and the decoder
   never touches a byte outside `[buffer, buffer+length)`.
2. **Only pass memory you may read at the current IRQL.** For user-mode code in a
   driver, probe and copy first (`MmCopyMemory`/`ProbeForRead` on Windows,
   `copy_from_user` on Linux) — lendiza never probes an address itself.

Working examples: [`kernel-examples/`](kernel-examples/README.md), including
`scripts/check_kernel_compile.cmd` (`cl /kernel` plus a `dumpbin` check that the
object references no CRT/RTTI/exception symbol) and
`scripts/check_kernel_compile.sh` (`g++ -ffreestanding -nostdinc++`, `nm -u`
check, and a real `make -C /lib/modules/.../build` when headers are present).

### Prefix semantics (important, and a change from pre-0.2 behaviour)
A prefix byte belongs to the instruction it precedes, so it is counted into the
returned length: `66 B8 34 12` is one 4-byte instruction, `48 B8 imm64` is one
10-byte instruction. `0x66` shrinks `Iz`/`rel32` fields to 16 bits, `REX.W`
widens only `MOV r,imm`, and `0x67` changes address size — moffs width in both
modes, plus the whole ModRM encoding (no SIB, disp16) in IA-32.

Consequences worth knowing:
- `0x66`, `0x67`, `0xF0`, one of `0xF2/0xF3` and REX may each appear once; a
  repeat returns `0xE1`. Segment overrides may stack (the last wins).
- `0x9B` (FWAIT) is decoded as its own 1-byte instruction, not as a prefix.
- VEX (`0xC4/0xC5`) and EVEX (`0x62`) encodings are **not** decoded in long mode
  and return `0xE1`; a linear scan over AVX code stops there. (In IA-32 those
  bytes are ordinary opcodes: `LES/LDS/BOUND`.)
- Total length is capped at 15 bytes; anything longer is reported `0xE1`.

### Examples
```bash
./build/lendiza_example_c
./build/lendiza_example_cpp
```

### Tests
`tests/README.md` documents the suites, the fixed decoder bugs this refactor
caught, the remaining known gaps (notably the IA-32 `0F A8-AF` table row), and
how to re-record the behaviour baseline in `tests/data/`.
