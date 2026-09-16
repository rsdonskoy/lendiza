#ifndef LENDIZA_COMMON_H
#define LENDIZA_COMMON_H

#include "lendiza_kernel.h"

#ifdef __cplusplus

/* Hosted code keeps the historical spellings; kernel code gets the same values
 * through the LDZ_ERR_* macros above. */
namespace lendiza
{

enum class ErrorCode {
    INSUFFICIENT_BUFFER = 0xE0,
    UNDEFINED_INSTRUCTION = 0xE1,
    UNKNOWN_ERROR = 0xE2
};

} // namespace lendiza

#endif /* __cplusplus */

#define LDZ_ERR_CODE(x) (static_cast<ldz_size>(lendiza::ErrorCode::x))

#ifdef __cplusplus

namespace lendiza::detail
{

    /* Trailing field of an encoding, i.e. what 0x66 (operand size) and 0x67
 * (address size) are able to resize. */
    enum tail_kind : ldz_u8 {
        tail_none = 0, /* nothing follows the opcode/ModRM */
        tail_ib,       /* imm8 - never resized */
        tail_iw,       /* imm16 - never resized */
        tail_iz,       /* imm16 / imm32 / imm64 - follows operand size */
        tail_iq,       /* imm16 / imm32 / imm64 - widens under REX.W (MOV r,imm) */
        tail_relz,     /* rel16 / rel32 - follows operand size */
        tail_moffs,    /* absolute address - follows address size */
        tail_far       /* ptr16:16 / ptr16:32 - follows operand size */
    };

    /* Entry encoding of the opcode-extension / x87 dispatch tables, see
     * docs/superpowers/specs/2026-09-15-lendiza-table-decode-design.md section 2. */
    constexpr ldz_u8 kGrpModrm = 0x00;    /* ModRM form, no trailing field */
    constexpr ldz_u8 kGrpTailFlag = 0x80; /* 0x80 | tail_kind */
    constexpr ldz_u8 kGrpFixedBase = 0xA0;/* 0xA0 | n: fixed body length n */

    /* rm field (bits 2-0) of a ModRM or SIB byte. */
    inline bool test_bits123_for(const ldz_u8 byte, const ldz_u8 bits)
    {
        return (byte & 0x07u) == static_cast<ldz_u8>(bits & 0x07u);
    }

    constexpr ldz_size SIZE_1_BYTE_IMMEDIATE = 1;
    constexpr ldz_size SIZE_2_BYTE_IMMEDIATE = 2;
    constexpr ldz_size SIZE_4_BYTE_IMMEDIATE = 4;
    constexpr ldz_size SIZE_8_BYTE_IMMEDIATE = 8;
    constexpr ldz_size SIZE_4_BYTE_DISPLACEMENT = 4;
    constexpr ldz_size SIZE_1_WORD_RELATIVE_OFFSET = 2;

    /* Bytes that ModRM + SIB + displacement add, given the ModRM byte.  The table
 * is written for the 32-bit and 64-bit addressing forms and counts the opcode
 * and the ModRM byte as well, so subtract 2 to get the trailing displacement.
 * A zero entry means "a SIB byte follows". */
    constexpr ldz_u8 kLengthTable_ModRM[256] = {
        /*      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
        /* 0 */ 2, 2, 2, 2, 0, 6, 2, 2, 2, 2, 2, 2, 0, 6, 2, 2,
        /* 1 */ 2, 2, 2, 2, 0, 6, 2, 2, 2, 2, 2, 2, 0, 6, 2, 2,
        /* 2 */ 2, 2, 2, 2, 0, 6, 2, 2, 2, 2, 2, 2, 0, 6, 2, 2,
        /* 3 */ 2, 2, 2, 2, 0, 6, 2, 2, 2, 2, 2, 2, 0, 6, 2, 2,
        /* 4 */ 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3,
        /* 5 */ 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3,
        /* 6 */ 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3,
        /* 7 */ 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3,
        /* 8 */ 6, 6, 6, 6, 7, 6, 6, 6, 6, 6, 6, 6, 7, 6, 6, 6,
        /* 9 */ 6, 6, 6, 6, 7, 6, 6, 6, 6, 6, 6, 6, 7, 6, 6, 6,
        /* A */ 6, 6, 6, 6, 7, 6, 6, 6, 6, 6, 6, 6, 7, 6, 6, 6,
        /* B */ 6, 6, 6, 6, 7, 6, 6, 6, 6, 6, 6, 6, 7, 6, 6, 6,
        /* C */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* D */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* E */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* F */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    };

    /* Same encoding as above for 16-bit addressing (0x67 in protected mode):
 * mod=00 rm=110 is disp16, mod=10 is disp16 rather than disp32, and no SIB
 * byte exists in this mode. */
    constexpr ldz_u8 kLengthTable_ModRM16[256] = {
        /*      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
        /* 0 */ 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
        /* 1 */ 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
        /* 2 */ 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
        /* 3 */ 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
        /* 4 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        /* 5 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        /* 6 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        /* 7 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        /* 8 */ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        /* 9 */ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        /* A */ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        /* B */ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        /* C */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* D */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* E */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* F */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    };

    /* Any value at or above this is an error code, never an instruction length. */
    inline constexpr bool is_error(const ldz_size value)
    {
        return value >= LDZ_ERR_BASE;
    }

    /* Operand size: 0x66 shrinks Iz/relz to 16 bits.  REX.W does not widen them -
 * forms like ADD r/m64, imm32 stay sign-extended imm32. */
    inline constexpr ldz_size imm_z_width(const bool has_66)
    {
        return has_66 ? SIZE_2_BYTE_IMMEDIATE : SIZE_4_BYTE_IMMEDIATE;
    }

    /* MOV r,imm (B8-BF) is the one form whose immediate really becomes 64 bits. */
    inline constexpr ldz_size imm_q_width(const bool has_66, const bool rex_w)
    {
        return rex_w ? SIZE_8_BYTE_IMMEDIATE : imm_z_width(has_66);
    }

    inline constexpr ldz_size rel_z_width(const bool has_66)
    {
        return imm_z_width(has_66);
    }

    /* Address size: 0x67 in long mode selects 32-bit addressing, 0x67 in
 * protected mode selects 16-bit addressing. */
    inline constexpr ldz_size moffs_width(const bool has_67, const bool long_mode)
    {
        return long_mode ? (has_67 ? SIZE_4_BYTE_IMMEDIATE : SIZE_8_BYTE_IMMEDIATE)
                   : (has_67 ? SIZE_2_BYTE_IMMEDIATE : SIZE_4_BYTE_IMMEDIATE);
    }

    /* Accumulated prefixes of one instruction.  Prefix bytes belong to the
 * instruction they precede, so their length is part of the answer. */
    struct prefix_info {
        ldz_u8 length;      /* bytes consumed by prefixes */
        ldz_u8 has_66;      /* operand-size override */
        ldz_u8 has_67;      /* address-size override */
        ldz_u8 has_seg;     /* segment override */
        ldz_u8 has_lock;    /* F0 */
        ldz_u8 has_rep;     /* F2 / F3 */
        ldz_u8 rex_present; /* long mode only */
        ldz_u8 rex_byte;    /* long mode only */

        bool rex_w() const { return (rex_byte & 0x08u) != 0u; }
        bool rex_r() const { return (rex_byte & 0x04u) != 0u; }
        bool rex_x() const { return (rex_byte & 0x02u) != 0u; }
        bool rex_b() const { return (rex_byte & 0x01u) != 0u; }
    };

    /* Bounds-checked view of the bytes a caller guarantees are readable.  The
 * decoder never reads past index size() - 1, which is what makes it safe to
 * call from a driver at DISPATCH_LEVEL. */
    class cursor {
    public:
        cursor(const ldz_u8* p, const ldz_size n) : p_(p), n_(n) {}

        ldz_size size() const { return n_; }
        bool has(const ldz_size count) const { return count <= n_; }
        ldz_u8 operator[](const ldz_size i) const { return p_[i]; }

    private:
        const ldz_u8* p_;
        ldz_size n_;
    };

    /* Table-driven length dispatch for the 0xEE (group) and 0xEF (x87)
     * sentinels of the opcode maps.  modrm_len and tail_width differ between
     * the IA-32 and long-mode traits, so the caller injects them. */
    template <typename ModrmLen, typename TailWidth>
    inline ldz_size group_len(const cursor& c, const ldz_size first, const ldz_size opc_size,
                              const prefix_info& pf, const ldz_u8 (&table)[256],
                              const ModrmLen modrm_len, const TailWidth tail_width)
    {
        if (!c.has(first + opc_size + 1)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        const ldz_u8 entry = table[c[first + opc_size]];
        if (is_error(static_cast<ldz_size>(entry))) {
            return static_cast<ldz_size>(entry);
        }
        /* Fixed body length; ldiza() runs the final buffer check.  The value
         * domain is locked by the GroupTableTest static_asserts, so the upper
         * bound is defence in depth: an encoding above the 0xA0|n band becomes
         * an explicit error instead of a silently truncated length. */
        if (entry >= kGrpFixedBase && entry < kGrpFixedBase + 0x10) {
            return static_cast<ldz_size>(entry & 0x0F);
        }
        if (entry >= kGrpFixedBase) {
            return LDZ_ERR_CODE(UNKNOWN_ERROR);
        }

        const ldz_size body = modrm_len(c, first, opc_size, pf);
        if (is_error(body)) {
            return body;
        }
        if ((entry & kGrpTailFlag) != 0) {
            return body + tail_width(static_cast<ldz_u8>(entry & 0x7F), 0, pf);
        }
        return body + static_cast<ldz_size>(entry);
    }

}

#endif /* __cplusplus */

#endif // !LENDIZA_COMMON_H
