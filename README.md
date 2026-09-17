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
check, a recompile with `-Dauto=__auto_type` to enforce the no-`auto` rule from
`lendiza_kernel.h`, and a real `make -C /lib/modules/.../build` when headers are
present).

### Prefix semantics (important, and a change from pre-0.2 behaviour)
A prefix byte belongs to the instruction it precedes, so it is counted into the
returned length: `66 B8 34 12` is one 4-byte instruction, `48 B8 imm64` is one
10-byte instruction. `0x66` shrinks `Iz`/`rel32` fields to 16 bits, `REX.W`
widens only `MOV r,imm`, and `0x67` changes address size — moffs width in both
modes, plus the whole ModRM encoding (no SIB, disp16) in IA-32.

Consequences worth knowing:
- `0x66`, `0x67`, `0xF0` and `0xF2`/`0xF3` may repeat freely: the CPU keeps one
  slot per prefix type, so a repeat only overwrites it while both bytes stay part
  of the instruction. `66 66 0F 1F 84 00 disp32` — the 10-byte NOP MSVC uses for
  16-byte function alignment — decodes as 10 bytes. Repeated REX bytes are
  likewise accepted, the last taking effect, and a legacy prefix after a REX
  voids that REX entirely (including its W/X/B/R bits), because only a REX
  adjacent to the opcode applies. Segment overrides stack (the last wins).
- `0x9B` (FWAIT) is decoded as its own 1-byte instruction, not as a prefix.
- An encoding whose ModRM names a register where the instruction requires a
  memory operand returns `0xE1`: the CPU cannot execute it at all, so there is no
  length to report. Measured on hardware for `0x8D` (LEA), whose whole `mod=11`
  band raises `EXCEPTION_ILLEGAL_INSTRUCTION` whatever the registers hold. This is
  decided from the bytes alone — an instruction that merely faults because of the
  *values* in its base registers (`8B 00` with a bad `rax`) still gets its length.
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

### Benchmark
`bench/bench_lde.cpp` sweeps real `.text` twice — once per decoder — and reports
elapsed time, throughput and successfully decoded instruction counts for the
same buffer. lde64 is chosen as the comparator because it is the only reference
available that is measured in the same units: it too returns nothing but an
instruction length. Timing lendiza against Zydis or Capstone would compare a
length lookup against a full decode and say nothing useful.

```bash
cmake -S . -B build-bench -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DLENDIZA_BUILD_TESTS=OFF -DLENDIZA_BUILD_BENCH=ON
cmake --build build-bench --target lendiza_bench
./build-bench/lendiza_bench.exe                            # ntdll + kernelbase + kernel32
./build-bench/lendiza_bench.exe --fixture ntdll --iters 80 # one corpus, more passes
./build-bench/lendiza_bench.exe --file some_dump.text.bin
```

The corpora are `.text` sections of Windows system DLLs; `LENDIZA_BENCH_FIXTURE_DIR`
points at the directory holding them and `LENDIZA_LDE64_LIB` at the lde64
archive. Both default to local checkouts, and the benchmark is off unless asked
for, since neither file is part of this repository.

3,751,936 bytes of `.text`, 40 passes, g++ 15.2 (MinGW-w64) Ninja Release:

| decoder | MiB/s | Minsn/s | decoded | errors | avg step |
|---|---:|---:|---:|---:|---:|
| lendiza | 209–262 | 62–78 | 42,558,520 | 17,480 | 3.62 B |
| lde64 | 77–96 | 23–29 | 42,556,960 | 21,720 | 3.53 B |

About 2.7x, holding between 2.6x and 2.9x across the individual fixtures. The
timing columns drift up to ~11% run to run; `decoded`, `errors` and `avg step`
are deterministic and identical across repeats. Two readings matter here:

- The decoders visit the same instruction boundaries — their counts differ by
  0.002% — so the ratio is per-decode cost rather than one walker stepping more
  times because it advances shorter.
- The `lendiza (C ABI)` row repeats the sweep through `lendiza_disasm_x64`
  because a header-only decoder called inline would otherwise be compared
  against a library call. It tracks the inlined row, so inlining is not the
  source of the gap.

Two lde64 traps, both handled by `bench/lde64_stub.s` but worth knowing before
calling it anywhere else:

- Its second argument is an **architecture selector** (`0` = IA-32, `64` =
  EM64T), not a buffer size. Passing anything except 64 silently decodes 64-bit
  code as IA-32, where every REX byte is a legitimate one-byte `inc`/`dec` and
  the result looks merely fast rather than wrong. It receives no bound at all, so
  the benchmark appends slack bytes after the corpus.
- Its `0x67` handler writes `ebx` without the prologue saving it, which violates
  the x64 callee-saved contract and surfaces as a crash long after the call, in
  whichever caller happened to keep a value in that register. The stub saves and
  restores it.
