// lendiza_decode_helpers.h - shorthand for "decode exactly these bytes".
//
// Every suite needs two things the raw API makes noisy: a byte-list literal and
// an explicit buffer length (truncation behaviour is half of what is tested).
// disasmN({...}) passes precisely the listed bytes, so a result of 0xE0 means
// the decoder refused to guess.
#ifndef LENDIZA_DECODE_HELPERS_H
#define LENDIZA_DECODE_HELPERS_H

#include <cstdint>
#include <initializer_list>
#include <vector>

#include "lendiza_core.hpp"

namespace lzdh {

constexpr size_t ERR_INSUFFICIENT = 0xE0;
constexpr size_t ERR_UNDEFINED = 0xE1;
constexpr size_t ERR_UNKNOWN = 0xE2;

inline std::vector<uint8_t> bytes(std::initializer_list<int> list)
{
    std::vector<uint8_t> v;
    v.reserve(list.size());
    for (const int b : list) {
        v.push_back(static_cast<uint8_t>(b));
    }
    return v;
}

/* Length of the instruction formed by these bytes, in 64-bit mode. */
inline size_t x64(std::initializer_list<int> list)
{
    const std::vector<uint8_t> v = bytes(list);
    return lendiza::detail::amd64traits::ldiza(v.data(), v.size());
}

/* Same for IA-32 (32-bit) mode. */
inline size_t x86(std::initializer_list<int> list)
{
    const std::vector<uint8_t> v = bytes(list);
    return lendiza::detail::x86traits::ldiza(v.data(), v.size());
}

} // namespace lzdh

/* ------------------------------------------------------------------------- *
 * An independent model of the Intel ModRM/SIB/displacement rules, written from
 * the manuals rather than from lendiza's tables, so a suite can derive an
 * expected length without repeating the implementation it is testing.
 * ------------------------------------------------------------------------- */
namespace lzmodel {

/* Opcodes whose ModRM must name a memory operand: the CPU rejects mod=11 for
 * them whatever the registers hold, so no length may be reported.  Measured for
 * 0x8D (LEA) as EXCEPTION_ILLEGAL_INSTRUCTION across the whole mod=11 band. */
inline bool requires_memory_operand(uint8_t opc)
{
    return opc == 0x8D;
}

/* Bytes contributed by ModRM + optional SIB + displacement, counted *after* the
 * opcode (the ModRM byte itself counts 1).  `rex_b` does not change any of it -
 * it selects a base register, never removes a displacement - and is kept only
 * because callers pass it; `addr16` selects 16-bit addressing (0x67 in
 * protected mode), where no SIB byte exists. */
inline size_t modrm_span(uint8_t modrm, uint8_t sib, [[maybe_unused]] bool rex_b, bool addr16)
{
    const unsigned mod = modrm >> 6;
    const unsigned rm = modrm & 7u;

    if (mod == 3u) {
        return 1; /* register operand: ModRM only */
    }

    if (addr16) {
        if (mod == 0u) {
            return rm == 6u ? 3 : 1; /* [disp16] versus a register-direct form */
        }
        return mod == 1u ? 2 : 3; /* disp8 / disp16 */
    }

    if (rm == 4u) {
        /* mod=00 with base=101 has no base register, so the disp32 is mandatory
         * even under REX.B; measured against the CPU, which consumes it. */
        const bool base5 = (sib & 7u) == 5u;
        const size_t disp = (mod == 0u && base5) ? 4u
                              : (mod == 1u ? 1u : (mod == 2u ? 4u : 0u));
        return 2 + disp; /* ModRM + SIB + optional displacement */
    }

    const size_t disp = (mod == 0u) ? (rm == 5u ? 4u : 0u) : (mod == 1u ? 1u : 4u);
    return 1 + disp; /* ModRM + optional displacement */
}

/* Immediate widths driven by operand size.  Iz is 32 bits, or 16 under 0x66;
 * it never becomes 64 bits, which only MOV r,imm does. */
inline size_t imm_ib()
{
    return 1;
}

inline size_t imm_iw()
{
    return 2;
}

inline size_t imm_iz(bool has_66)
{
    return has_66 ? 2u : 4u;
}

inline size_t imm_iq(bool has_66, bool rex_w)
{
    return rex_w ? 8u : imm_iz(has_66);
}

} // namespace lzmodel

#endif // LENDIZA_DECODE_HELPERS_H
