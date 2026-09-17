# lendiza Test Suite

Zero-dependency suites: `tests/lendiza_test.h` provides a GoogleTest-shaped API
(`TEST`, `EXPECT_EQ`, `EXPECT_TRUE`) plus a `--filter=` runner, so everything
builds on a bare toolchain and can be switched to real GoogleTest by swapping
that one include. Suite names, file split, and the "assert current behaviour for
known gaps" convention follow `xendiza/test/`.

## Test Organization

### `test_public_api.cpp` — `PublicApiTest`
Every hosted entry point: `std::span` call operator, the polymorphic
`lendiza::ldiza::ldisasm`, the inline core helpers (`lendiza::disasm_x64/x86`),
the `extern "C"` ABI, null/empty buffers, agreement between C and C++ error
codes, and that no state survives a call (a REX-prefixed instruction must not
change the answer for the next one).

### `test_prefixes.cpp` — `PrefixTest`, `PrefixTest32`
The prefix *model*: which bytes are prefixes, that they count into the returned
length, REX appearing at most once, duplicate `66/67/F0/F2-F3` rejected, segment
overrides stacking (last wins), `0F` being an opcode escape rather than a
prefix, VEX/EVEX rejected in long mode but being ordinary opcodes in IA-32, and
the 15-byte instruction limit.

### `test_prefix_operand_size.cpp` — `Prefix66Test`, `Prefix66Test32`
0x66 (operand size): every `Iz` family shrinks (`05/0D/...`, `81`, `69`, `A9`,
`68`, `F7`, `C7`), `rel32` → `rel16` (`E8/E9`, `0F 80-8F`), `MOV r,imm`
(`B8-BF`) with `REX.W` applying only when it is the last prefix before the
opcode (an `0x66` that follows it wins instead), the IA-32 far `9A/EA` forms, and
the things 0x66 must *not* resize (imm8, displacements, SIB, moffs).

### `test_prefix_address_size.cpp` — `Prefix67Test`, `Prefix67Test32`
0x67 (address size): moffs 8→4 in long mode and 4→2 in IA-32; SIB base=101
keeping its disp32 with or without `REX.B`; RIP-relative staying disp32;
and in IA-32 the switch to 16-bit addressing (no SIB byte at all, `rm=110`
becoming a disp16, `mod=10` becoming disp16).

### `test_basic_instructions.cpp` / `test_x86_basic.cpp` — `DisasmTest`, `X86BasicTest`
One-byte opcode map: the `Eb/Gb`, `Ev/Gv`, `AL, Ib` and `eAX, Iz` families,
`PUSH/POP` short forms, `Jcc rel8`, moffs, string operations, fixed-length ops,
plus the IA-32-only forms (`PUSH AD/POPAD`, segment `PUSH/POP`, `PUSH FS/GS`,
BCD adjusts, far `9A/EA`) that long mode rejects.
Expected lengths come from `lzmodel::modrm_span()` — an independent statement of
the Intel ModRM/SIB/displacement rules — never from the library's tables. Each
form is also decoded one byte short to confirm it reports `INSUFFICIENT_BUFFER`
instead of guessing.

### `test_opcode_extensions.cpp` — `OpcodeExtTest`
Groups 1-6 (`80-83`, `C0-C1`, `D0-D3`, `C6-C7`, `F6-F7`, `FE-FF`) and `69/6B`
IMUL: 512 prefix × ModRM × reg-field combinations per opcode, with the
immediate-width rule written out per Intel group definition.

### `test_2byte_opcodes.cpp` — `TwoByteOpcodeTest`
`0F`-prefixed families (SSE moves, `MOVZX/MOVSX`, `BSR/BSF`, `LSS/BSS/LSS`,
`SHLD/SHRD`, `BT/BTS/BTR/BTC`, `XADD`, `CMPXCHG`, `IMUL 0F AF`, `BSWAP`), the
`0F 00/01/AE/BA/C7/71-73` extension groups, the `0F 38`/`0F 3A` three-byte
escapes, and the cells the map marks undefined (with their defined neighbours,
so the list cannot be an off-by-one).

### `test_x87_opcodes.cpp` — `X87OpcodeTest`, `X86x87Test`
`D8-DF`: 432 memory forms per mode through the independent model, 512 register
forms per mode asserted to be exactly two bytes or an explicit rejection, the
named register forms (`FCHS`, `FLD1`, `FST`, `FNSTSW AX`, ...), and the three
reserved extensions (`D9 /1`, `DB /4`, `DB /6`).

### `test_system_opcodes.cpp` — `SystemOpcodeTest`, `SystemOpcodeTest32`
Control-register transfers, `MOV to/from DR`, the no-operand control forms
(`SYSCALL`, `SYSRET`, `CLTS`, `INVD`, `WBINVD`, `CPUID`, `RDMSR`, `WRMSR`,
`RDTSC`, `SYSENTER/SYSEXIT`), `IN/OUT` in both immediate and DX forms, flag ops,
and the `0F 01 /2-/6` memory forms (lgdt/lidt/sgdt/sidt) whose displacement the
old decoder dropped.

### `test_boundary.cpp` — `BoundaryTest`, `BoundaryTest32`
The kernel-safety invariant stated over the whole opcode space:
**for every input and every buffer length `n`, the result is either `<= n` or an
error code** — plus the named boundaries (ModRM implying a SIB, RIP-relative
disp32, `REX.B` + SIB base=101 still needing its disp32, imm16/32/64, group immediates,
three-byte escapes) and prefix-only / 15-byte-limit cases.

### `test_oracle_regression.cpp` — `OracleTest`
Replays `tests/data/oracle_before.bin` (718,297 cases per mode, recorded from the
pre-kernel-port decoder over `tests/lendiza_corpus.h`) and fails on any difference
outside the seven reviewed categories (the newest, `reg-operand-invalid`, covers
encodings the CPU cannot execute because the ModRM names a register where the
instruction requires memory - `8D C0`..`8D FF`). Also checks that unprefixed, complete
encodings are bit-identical (90,941 cases), that a linear walk over real code
covers at least as much as before with fewer steps, and that widening the buffer
never changes a complete answer (52,459 comparisons).

### `test_group_oracle.cpp` — `GroupOracleTest`
The main gate of the table refactor. It replays the exhaustive sweep of the
`0xEE`/`0xEF` dispatch space in `lendiza_group_sweep.h` — every group table
(`C6/C7/F6/F7/FE/FF`, `0F 00/01/18/71/72/73/AE/B9/BA/C7`) and every x87 row,
11 prefix sets (4 IA-32 + 7 long-mode; the last two long-mode sets carry REX.B,
which the sweep previously never set and which is what let a wrong REX.B/SIB rule
pass this oracle unnoticed) × 24 group/x87 cells × 256 ModRM bytes ×
4 SIB/filler shapes × 7 buffer windows = 1,892,352 cases — against
`tests/data/group_before.bin` and fails on **any** difference: unlike
`OracleTest` there is no exemption category, because the refactor is
behaviour-preserving by contract. The baseline is recorded from the
*pre-refactor* decoder with:

```bash
cmake --build build --target lendiza_dump_group_baseline
./build/lendiza_dump_group_baseline.exe tests/data/group_before.bin [--force]
```

The dumper refuses to overwrite an existing file; `--force` exists only for a
reviewed, intended behaviour change and is the point where that review gets
stated out loud. Reordering or inserting sweep cases changes the baseline
format — see the guard note at the top of `lendiza_group_sweep.h`.

### `test_group_tables.cpp` — `GroupTableTest`
Compile-time value-domain check for the 34 dispatch arrays of
`lendiza_x86_tables.hpp` / `lendiza_amd64_tables.hpp` (16 group tables plus one
`[8][256]` x87 table per architecture): every entry must be one of the
encodings of spec section 2 — error codes `0xE0-0xE2`, fixed bodies `0xA0|n`
with n in [2,15], the resized tail `0x83` (`0x80|tail_iz`, the only one in
use), or plain additions `0x00/0x01/0x02/0x04/0x08` — so a hand-pasted typo
cannot survive as a silent runtime error. A second set of asserts pins the
`0xEF`/`0xEE` sentinels of the four main opcode→length maps to exactly the
cells whose dispatch can service them, so editing a main map cannot turn into
an out-of-range x87 row index.

### Group dispatch tables — `lendiza_x86_tables.hpp` / `lendiza_amd64_tables.hpp`
Entry-value encoding: spec section 2
(`docs/superpowers/specs/2026-09-15-lendiza-table-decode-design.md`). The table
data is produced by the one-off generator script of the refactor (which
mirrors the pre-refactor switch predicates in order) and is deliberately not
kept in the repo. **Never hand-edit table entries**: change the rules, then
regenerate the affected whole table, and let `GroupTableTest`,
`GroupOracleTest` and `OracleTest` grade it. The committed `group_before.bin`
predates these two files: the pre-refactor (switch) decoder recorded it, and
the tables plus their dispatch wiring were afterwards verified by
`GroupOracleTest` to reproduce that recording byte for byte (1,548,288 cases,
diffs=0). It is that verified equality a hand-edited entry has to survive, so
a table edit that is not script-regenerated will show up there first.

### `guard_page.cpp`, `asan_truncation.c` — standalone probes, not gtest cases
`guard_page.cpp` puts every input against a `PAGE_NOACCESS`/`PROT_NONE` page and
turns a fault into a reported violation (2.76M decodes, 0 over-reads). It starts
with a negative control — an intentional one-past-the-end read must fault, or the
probe exits 3 rather than report a meaningless zero. Run against the
pre-refactor headers the same harness reports **103 over-reads**, which is what
makes the current 0 worth anything.
`asan_truncation.c` runs the same matrix through the `extern "C"` ABI with each
buffer allocated at exactly its own length for AddressSanitizer (2.6M decodes).

## Fixed Decoder Bugs

Surfaced by these suites and fixed in the core. The old implementation kept REX
in a mutable `inline static` member, so every one of these was also unsafe on SMP:

1. **Prefixes were returned as their own 1-byte "instruction".** `66 B8 34 12`
   answered 1, `48 B8 imm64` answered 1, and `B8` on its own answered whatever a
   previous call had left in `rex_prefix_` (9 for a stale `REX.W`). Now prefixes
   are consumed by `ProcessPrefix` and counted into the instruction.
   (Tests: `Prefix66Test.MOV_0xB8_*`, `PrefixTest.REX_*`, `OracleTest`.)
2. **Truncated buffers were read past their end.** The ModRM helper fetched a
   SIB byte the caller had not guaranteed (2 bytes in, length 7 out). Every fetch
   is now bounds-checked and a window too small returns `0xE0`.
   (Tests: `BoundaryTest.*`, `guard_page.cpp`.)
3. **Group 5/4 handlers dropped the displacement.** `F6 05 ...` (test rip+disp32)
   answered 2; `F7/C6/C7/FE/FF` did the same. They now use the ModRM path.
   (Tests: `OpcodeExtTest.Group*`, oracle category `group-disp`.)
4. **`0F 00`/`0F 01` mis-measured their own opcode and operands.** The x64 handler
   counted one opcode byte (3 instead of 4 for `0F 00 /r`); the IA-32 handler added
   two, and both returned 3 for `sgdt/sidt` memory forms that carry disp8/disp32.
   (Tests: `TwoByteOpcodeTest`, `SystemOpcodeTest.Lgdt_sidt_memory_forms`.)
5. **`amd64traits::k_tail_table` had `tail_iz` on the wrong cell** (`0x83`, an
   imm8 form, instead of `0x81`, the `Iz` form), so `66 81 C0 ...` over-counted.
   (Test: `Prefix66Test.Group1_0x81_Iz_imm16`.)
6. **The IA-32 two-byte map marked row `0F A8`-`0F AF` as six-byte forms.** Correct
   widths (matching the long-mode row, which was already right): `0F A8/A9`
   PUSH/POP GS and `0F AA` RSM are 2 bytes; `0F AB` BTS, `0F AD` SHRD-CL and
   `0F AF` IMUL are ModRM forms; `0F A4` SHLD and `0F AC` SHRD are ModRM + imm8.
   `0F A4`/`0F AC` were also marked `0x00` (plain ModRM), and the IA-32 escape
   handler had no `0xA1` case at all, so that category was added to
   `process_2byte_opc`.
   (Tests: `SystemOpcodeTest.Row_a_lengths_agree_across_modes` — added precisely
   because a row that differs between the two maps is a table bug, not a mode
   difference, and no single-mode test could see it — plus
   `DisasmTest.Scas_and_imul_are_not_confused`,
   `SystemOpcodeTest32.Push_pop_segment_and_far_returns_exist`. Oracle category
   `ia32-row-a`, 776 cases, zero in long mode.)

## Known Gaps (asserted at current behaviour, not fixed here)

- **VEX (`C4/C5`) and EVEX (`62`) are not decoded**, matching xendiza: they return
  `0xE1`. A scan through AVX code stops at the first VEX instruction.
- **`0x9B` (FWAIT) is not treated as a prefix**, so it stays its own 1-byte
  instruction (same set as xendiza).
- **`0F F7` (MASKMOVQ)** is a fixed 3-byte form in the map, so a memory-looking
  ModRM under it does not add a displacement.

## Test Statistics

- 166 tests, 1,347 assertions, 0 failures (`lendiza_tests`).
- Inside the loops: ~8,700 model-derived encodings (512 per group opcode, 432
  x87 memory forms and 512 register forms per mode, 644 two-byte encodings,
  ~4,000 ModRM/prefix combinations across the basic suites).
- 1,436,594 oracle cases (718,297 per mode), 1,548,288 zero-diff group-sweep
  cases (`GroupOracleTest`), and 52,459 window-invariance comparisons.
- 2.76M guard-page decodes and 2.6M ASan decodes, both with zero over-reads.
- Run `ctest --output-on-failure` for current results; `ctest -L quick` skips the
  two long probes.

## Running Tests

```bash
cmake -S . -B build -G Ninja && cmake --build build
ctest --test-dir build -L quick --output-on-failure      # suites
ctest --test-dir build -L kernel-safety                   # guard page + ASan matrix
ctest --test-dir build --output-on-failure               # everything

# directly
./build/lendiza_tests
./build/lendiza_tests --filter=Prefix67

# without CMake (GCC/MinGW), run from the repository root; the three data macros
# are spelled from there because the compiled-in defaults are relative to tests/
g++ -std=c++20 -Iinclude -Itests \
    -DLENDIZA_ORACLE_PATH='"tests/data/oracle_before.bin"' \
    -DLENDIZA_WALK_PATH='"tests/data/walk_before.txt"' \
    -DLENDIZA_GROUP_BASELINE_PATH='"tests/data/group_before.bin"' \
    tests/test_*.cpp src/lendiza.cpp -o lendiza_tests && ./lendiza_tests
```

Kernel-side checks are separate because they need a kernel toolchain:

```bat
scripts\check_kernel_compile.cmd      :: cl /kernel + dumpbin symbol check
sh   scripts/check_kernel_compile.sh  :: g++ -ffreestanding -nostdinc++ (+ kbuild if headers exist)
tests\tools\msvc_asan.cmd             :: ASan over-read matrix
```

Re-recording the baseline (only when a behaviour change is intended and reviewed):

```bash
g++ -std=c++20 -I../include -I.. tools/dump_baseline.cpp -o dump && ./dump ../data/oracle_before.bin
```

## Naming Convention

`{MNEMONIC}_{OPCODE}_{PREFIX}_{OPERANDS}`, with the length stated in the name for
prefix tests:

- `MOV_0xB8_REX_W_imm64` - `48 B8 imm64` is 10 bytes
- `Prefix66Test.MOV_0xB8_Prefix66_imm16` - `66 B8 imm16` is 4 bytes
- `Prefix67Test.Sib_base5_keeps_disp32_with_or_without_REX_B`
- `BoundaryTest.Immediates_need_every_byte`

## Coverage Analysis

### Implemented & tested
- 1-byte opcode map in both modes, including the mode-exclusive forms
- Groups 1-6 and the `0F 00/01/AE/BA/C7/71/72/73/B9` extension groups
- Two-byte escapes and the `0F 38` / `0F 3A` three-byte families
- x87 `D8-DF`, register and memory forms
- 0x66, 0x67, REX, LOCK, REP/REPE, segment overrides, duplicate rejection, 15-byte cap
- Every truncation boundary and the no-over-read invariant, proved twice
- C ABI and hosted C++ API equivalence

### Not yet tested
- IA-32 opcode-extension groups and two-byte families as separate suites (the
  x86 side is covered by `test_x86_basic.cpp`, `test_prefix_*`, `test_x87_opcodes.cpp`
  and `test_system_opcodes.cpp`, but not with the 512-combination group sweep)
- Real-mode and VM86 encodings (out of scope: the decoder targets 32/64-bit modes)
- 0F 38/0F 3A opcode coverage beyond the forms listed above
