// kernel_compile_probe.cpp - the smallest possible kernel user: it includes only
// the kernel-safe umbrella and calls the decoder.  Compiled by
// scripts/check_kernel_compile.cmd (cl /kernel) and
// scripts/check_kernel_compile.sh (g++ -ffreestanding -nostdinc++), so a failure
// here means the core itself is not portable, not that a sample is misconfigured.
//
// Deliberately no <cstring>, no STL, no new/delete, no exceptions: if the core
// ever grows a dependency on any of them, this file stops compiling.
#include "lendiza_core.hpp"

// Sizes a code window: enough for the longest instruction, prefixes included.
static const ldz_size kWindow = LDZ_MAX_INSNS_LEN;

extern "C" ldz_size lendiza_probe_decode(const ldz_u8* bytes, ldz_size n, int long_mode)
{
    return long_mode ? lendiza::disasm_x64(bytes, n) : lendiza::disasm_x86(bytes, n);
}

extern "C" ldz_size lendiza_probe_walk(const ldz_u8* bytes, ldz_size n, int long_mode)
{
    ldz_size offset = 0;

    while (offset < n) {
        const ldz_size remaining = n - offset;
        const ldz_size window = remaining < kWindow ? remaining : kWindow;
        const ldz_size len = long_mode ? lendiza::disasm_x64(bytes + offset, window)
                                       : lendiza::disasm_x86(bytes + offset, window);
        if (lendiza::is_error(len) || len == 0) {
            break;
        }
        offset += len;
    }
    return offset;
}

// Compile-time checks that the decode entry points stay usable in kernels.
static_assert(sizeof(ldz_u8) == 1, "ldz_u8 must be one byte in every environment");
static_assert(LDZ_MAX_INSNS_LEN == 15u, "x86 instructions are capped at 15 bytes");
static_assert(LDZ_ERR_INSUFFICIENT_BUFFER == 0xE0u, "error codes are part of the ABI");
static_assert(LDZ_ERR_BASE > LDZ_MAX_INSNS_LEN, "lengths and error codes must not overlap");
