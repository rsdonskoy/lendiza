// lendiza_kshim.cpp - the C++ half of the Linux example, deliberately free of
// kernel headers.
//
// The decode core needs no kernel API: it reads a byte window and returns a
// length.  Including <linux/*.h> here would couple it to the kernel's own C++
// support, which only exists in-tree since v6.15 - pre-6.15 headers fail to
// compile as C++ (rwonce.h's __unqual_scalar_typeof, `auto` defined as
// __auto_type, `inline` expanded to `__gnu_inline`).  Keeping them out means this
// TU builds with plain -ffreestanding -nostdinc++ and links into the module like
// any other object; the C file, which does use kernel headers, turns the raw
// results into errno values.
//
// Compile flags live in Makefile.

#include "lendiza_core.hpp"

/* Raw decoder results, passed straight through to the C side:
 * 0..15          instruction length
 * LDZ_ERR_*      0xE0 not enough bytes, 0xE1 undecodable, 0xE2 internal
 *
 * extern "C": the C half of the module refers to these unmangled, and modpost
 * resolves symbols, not types. */
extern "C" {

long lendiza_kshim_decode(const ldz_u8* bytes, ldz_size n, int long_mode)
{
    const ldz_size len = long_mode ? lendiza::disasm_x64(bytes, n)
                                   : lendiza::disasm_x86(bytes, n);
    return static_cast<long>(len);
}

/* Walks a code region; stores the instruction count and returns bytes consumed.
 * Stops at the first undecodable or truncated encoding. */
ldz_size lendiza_kshim_walk(const ldz_u8* bytes, ldz_size n, int long_mode, ldz_size* count)
{
    ldz_size offset = 0;
    ldz_size decoded = 0;

    if (count != nullptr) {
        *count = 0;
    }

    while (offset < n) {
        const ldz_size remaining = n - offset;
        const ldz_size window = remaining < LDZ_MAX_INSNS_LEN ? remaining : LDZ_MAX_INSNS_LEN;
        const ldz_size len = lendiza_kshim_decode(bytes + offset, window, long_mode);

        if (lendiza::is_error(len) || len == 0) {
            break;
        }
        offset += len;
        ++decoded;
    }

    if (count != nullptr) {
        *count = decoded;
    }
    return offset;
}

} // extern "C"

/* The core must stay usable with nothing but a freestanding compiler. */
static_assert(sizeof(ldz_u8) == 1, "ldz_u8 must be one byte");
static_assert(LDZ_MAX_INSNS_LEN == 15u, "x86 instructions are capped at 15 bytes");
static_assert(LDZ_ERR_BASE > LDZ_MAX_INSNS_LEN, "lengths and error codes must not overlap");
