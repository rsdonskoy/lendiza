// test_2byte_opcodes.cpp
// 0F-prefixed (two-byte) opcode coverage for x86-64, plus the 0F 38 / 0F 3A
// three-byte escapes.
//
// Each row states, from the Intel manuals, whether the encoding carries a ModRM
// byte and what immediate follows it; the expected length is then
//     prefixes + 2 (opcode bytes) + ModRM/SIB/displacement + immediate
// using the independent model in lendiza_decode_helpers.h.  A one-byte-short
// copy of every encoding must report INSUFFICIENT.
//
// Suites: TwoByteOpcodeTest.

#include <cstdint>
#include <vector>

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

namespace {

size_t decode64(const uint8_t* p, size_t n)
{
    return lendiza::detail::amd64traits::ldiza(p, n);
}

enum imm_kind : uint8_t {
    imm_none = 0,
    imm_8,   /* one immediate byte */
    imm_16,  /* word immediate (never resized by 0x66 here) */
    imm_32   /* Iz / rel32: shrinks to 16 bits under 0x66 */
};

struct Form {
    uint8_t second;   /* byte after 0F */
    bool has_modrm;
    uint8_t imm;      /* imm_kind */
    const char* what; /* the mnemonic family, for failure output */
};

/* Assembled from the SDM instruction set, not from lendiza's tables. */
const std::vector<Form>& forms()
{
    static const std::vector<Form> f = {
        { 0x10, true, imm_none, "MOVUPS xmm, xmm/m128" },
        { 0x11, true, imm_none, "MOVUPS m128, xmm" },
        { 0x12, true, imm_none, "MOVUPS/MOVLPS" },
        { 0x13, true, imm_none, "MOVUPSLPS" },
        { 0x14, true, imm_none, "UNPCKLPS" },
        { 0x15, true, imm_none, "UNPCKHPS" },
        { 0x16, true, imm_none, "MOVHPS" },
        { 0x17, true, imm_none, "MOVHPS m" },
        { 0x28, true, imm_none, "MOVAPS" },
        { 0x29, true, imm_none, "MOVAPS m" },
        { 0x2C, true, imm_none, "CVTTS2SI" },
        { 0x2E, true, imm_none, "UCOMISS" },
        { 0x40, true, imm_none, "CMOVO" },
        { 0x4F, true, imm_none, "CMOVG" },
        { 0x50, true, imm_none, "MOVMSKPS" },
        { 0x51, true, imm_none, "SQRTPS" },
        { 0x54, true, imm_none, "ANDPS" },
        { 0x57, true, imm_none, "XORPS" },
        { 0x5E, true, imm_none, "DIVPS" },
        { 0x6E, true, imm_none, "MOVD r/m32, xmm" },
        { 0x6F, true, imm_none, "MOVQ" },
        { 0x7E, true, imm_none, "MOVQ/MOVD" },
        { 0x7F, true, imm_none, "MOVMA" },
        { 0xA2, false, imm_none, "CPUID" },
        { 0xA3, true, imm_none, "BT r/m, r" },
        { 0xA4, true, imm_8, "SHLD r/m, r, imm8" },
        { 0xA5, true, imm_none, "SHLD r/m, r, CL" },
        { 0xA8, false, imm_none, "PI2FD" },
        { 0xAA, false, imm_none, "RSM" },
        { 0xAB, true, imm_none, "BTS r/m, r" },
        { 0xAC, true, imm_8, "SHRD r/m, r, imm8" },
        { 0xAD, true, imm_none, "SHRD r/m, r, CL" },
        { 0xB0, true, imm_none, "CMPXCHG b" },
        { 0xB1, true, imm_none, "CMPXCHG v" },
        { 0xB6, true, imm_none, "MOVZX r, r/m8" },
        { 0xB7, true, imm_none, "MOVZX r, r/m16" },
        { 0xBE, true, imm_none, "MOVSX r, r/m8" },
        { 0xBF, true, imm_none, "MOVSX r, r/m16" },
        { 0xC0, true, imm_none, "XADD b" },
        { 0xC1, true, imm_none, "XADD v" },
        { 0xC3, true, imm_none, "MOV RX, r64" },
        { 0xC4, true, imm_8, "PINSRW" },
        { 0xC5, true, imm_8, "PEXTRW" },
        { 0xC6, true, imm_8, "SHUFPS" },
        { 0xC8, false, imm_none, "BSWAP eax" },
        
        { 0xD4, true, imm_none, "PSADDQ" },
        /* 0F F7 (MASKMOVQ) is a fixed 3-byte form here; covered explicitly below. */
    };
    return f;
}

size_t imm_bytes_of(uint8_t kind, bool has_66)
{
    switch (kind) {
    case imm_8: return 1;
    case imm_16: return 2;
    case imm_32: return has_66 ? 2u : 4u;
    default: return 0;
    }
}

/* Builds [0F][second][ModRM][SIB][disp][imm] and checks both the full and the
 * one-byte-short answer. */
void check_form(const Form& f, uint8_t modrm, uint8_t sib, size_t& failures, size_t& combos,
                bool has_66)
{
    std::vector<uint8_t> v;
    if (has_66) {
        v.push_back(0x66);
    }
    v.push_back(0x0F);
    v.push_back(f.second);

    size_t tail = 0;
    if (f.has_modrm) {
        const bool uses_sib = (modrm >> 6) != 3u && (modrm & 7u) == 4u;
        v.push_back(modrm);
        if (uses_sib) {
            v.push_back(sib);
        }
        const size_t span = lzmodel::modrm_span(modrm, sib, false, false);
        const size_t header = uses_sib ? 2u : 1u;
        if (span < header) {
            return;
        }
        for (size_t i = 0; i < span - header; ++i) {
            v.push_back(static_cast<uint8_t>(0x50 + i));
        }
        tail = span;
    }
    const size_t imm = imm_bytes_of(f.imm, has_66);
    for (size_t i = 0; i < imm; ++i) {
        v.push_back(static_cast<uint8_t>(0xA0 + i));
    }

    ++combos;
    const size_t expected = (has_66 ? 1u : 0u) + 2u + tail + imm;
    if (v.size() > LDZ_MAX_INSNS_LEN) {
        return;
    }
    const size_t got = decode64(v.data(), v.size());
    if (got != expected) {
        ++failures;
        std::printf("    0F %02X modrm=%02X (%s) expected=%zu got=%zu\n", f.second, modrm, f.what,
                    expected, got);
    }
    if (v.size() >= 3) {
        std::vector<uint8_t> cut(v.begin(), v.end() - 1);
        const size_t got_cut = decode64(cut.data(), cut.size());
        if (got_cut != ERR_INSUFFICIENT) {
            ++failures;
            std::printf("    truncated 0F %02X modrm=%02X n=%zu -> %zu\n", f.second, modrm, cut.size(),
                        got_cut);
        }
    }
}

} // namespace

TEST(TwoByteOpcodeTest, ModRM_families_across_all_mod_modes)
{
    size_t failures = 0;
    size_t combos = 0;
    static const uint8_t modrms[] = { 0x00, 0x05, 0x04, 0x44, 0x84, 0xC0, 0xFF };

    for (const Form& f : forms()) {
        for (const uint8_t modrm : modrms) {
            for (const bool has_66 : { false, true }) {
                check_form(f, modrm, 0x25, failures, combos, has_66);
            }
        }
    }
    std::printf("    %zu encodings checked\n", combos);
    EXPECT_EQ(failures, 0u);
    EXPECT_TRUE(combos > 600u);
}

TEST(TwoByteOpcodeTest, Relative_jumps_0x80_to_0x8F)
{
    for (uint8_t second = 0x80; second <= 0x8F; ++second) {
        EXPECT_EQ(x64({ 0x0F, second, 1, 2, 3, 4 }), 6u);              /* rel32 */
        EXPECT_EQ(x64({ 0x66, 0x0F, second, 1, 2 }), 5u);              /* rel16 under 0x66 */
        EXPECT_EQ(x64({ 0x0F, second, 1, 2, 3 }), ERR_INSUFFICIENT);
    }
}

TEST(TwoByteOpcodeTest, Setcc_and_test_groups_take_ModRM)
{
    for (uint8_t second = 0x90; second <= 0x9F; ++second) {
        EXPECT_EQ(x64({ 0x0F, second, 0xC0 }), 3u);   /* SETcc r/m8 */
        EXPECT_EQ(x64({ 0x0F, second, 0x05, 1, 2, 3, 4 }), 7u); /* disp32 form */
    }
}

TEST(TwoByteOpcodeTest, Control_register_moves)
{
    static const uint8_t cr_ops[] = { 0x20, 0x21, 0x22, 0x23 };
    for (const uint8_t second : cr_ops) {
        EXPECT_EQ(x64({ 0x0F, second, 0xC0 }), 3u);        /* MOV Cn/Dr, r64 */
        EXPECT_EQ(x64({ 0x48, 0x0F, second, 0xC0 }), 4u);  /* REX counted in */
        EXPECT_EQ(x64({ 0x0F, second }), ERR_INSUFFICIENT);
    }
    EXPECT_EQ(x64({ 0x0F, 0x25 }), ERR_UNDEFINED); /* the retired 0F 25 slot */
}

TEST(TwoByteOpcodeTest, Extension_groups_select_on_reg_field)
{
    /* 0F 00: sldt/str/lldt/ltr - register and memory forms both exist. */
    EXPECT_EQ(x64({ 0x0F, 0x00, 0xC0 }), 3u);
    EXPECT_EQ(x64({ 0x0F, 0x00, 0x00 }), 3u);
    EXPECT_EQ(x64({ 0x0F, 0x00, 0x38 }), ERR_UNDEFINED); /* /7 is not defined */

    /* 0F 01: sgdt/sidt (memory, so displacements must be counted) and the
     * register-only monitor/mwait/invlpg family. */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x24, 0x00 }), 4u);   /* sgdt [SIB] */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x25, 1, 2, 3, 4 }), 7u); /* sgdt [rip+disp32] */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0xC8 }), 3u);         /* monitor */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0xC9 }), 3u);         /* mwait */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0xD8 }), ERR_UNDEFINED);

    /* 0F BA: bt/bts/btr/btc with imm8, only /4-/7 defined. */
    EXPECT_EQ(x64({ 0x0F, 0xBA, 0xE0, 0x08 }), 4u);
    EXPECT_EQ(x64({ 0x0F, 0xBA, 0xE8, 0x08 }), 4u);
    EXPECT_EQ(x64({ 0x0F, 0xBA, 0xF8, 0x08 }), 4u);
    EXPECT_EQ(x64({ 0x0F, 0xBA, 0xC0 }), ERR_UNDEFINED); /* /0 is not defined */

    /* 0F AE: fxsave/xsave take memory, lfence/mfence/sfence are register-only. */
    EXPECT_EQ(x64({ 0x0F, 0xAE, 0x10 }), 3u);
    EXPECT_EQ(x64({ 0x0F, 0xAE, 0xE8 }), 3u);      /* lfence */
    EXPECT_EQ(x64({ 0x0F, 0xAE, 0xF0 }), 3u);      /* mfence */
    EXPECT_EQ(x64({ 0x0F, 0xAE, 0x05, 1, 2, 3, 4 }), 7u); /* fxsave [rip+d32] */

    /* 0F 71/72/73: MMX shift groups, imm8 only in the register form. */
    EXPECT_EQ(x64({ 0x0F, 0x71, 0xE2, 0x04 }), 4u);
    EXPECT_EQ(x64({ 0x0F, 0x71, 0x10 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x0F, 0x73, 0xF2, 0x04 }), 4u);
}

TEST(TwoByteOpcodeTest, Three_byte_escapes)
{
    /* 0F 38 /0 through /f and friends: opcode is 3 bytes, then ModRM. */
    EXPECT_EQ(x64({ 0x0F, 0x38, 0x00, 0xC1 }), 4u);   /* PSHUFB mm, mm/m128 */
    EXPECT_EQ(x64({ 0x0F, 0x38, 0xF0, 0xC1 }), 4u);   /* MOVBE */
    EXPECT_EQ(x64({ 0x0F, 0x38, 0xF1, 0xC1 }), 4u);   /* MOVBE reverse */
    EXPECT_EQ(x64({ 0x0F, 0x3A, 0x0F, 0xC1, 0x02 }), 5u); /* PALIGNR mm, mm, imm8 */
    EXPECT_EQ(x64({ 0x0F, 0x3A, 0x14, 0xC1, 0x02 }), 5u); /* PEXTRB */
    EXPECT_EQ(x64({ 0x0F, 0x38, 0x1C }), ERR_INSUFFICIENT);  /* not encodable here */
    EXPECT_EQ(x64({ 0x0F, 0x3A, 0x00 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x0F, 0x38 }), ERR_INSUFFICIENT);
}

TEST(TwoByteOpcodeTest, Bswap_family_0xC8_to_0xCF)
{
    /* BSWAP is a two-byte form here; the SHA instructions live under 0F 38 C8-D1
     * and are covered by the three-byte tests above. */
    for (uint8_t second = 0xC8; second <= 0xCF; ++second) {
        EXPECT_EQ(x64({ 0x0F, second, 0xC0 }), 2u);
        EXPECT_EQ(x64({ 0x48, 0x0F, second }), 3u); /* REX belongs to the BSWAP */
    }
}

TEST(TwoByteOpcodeTest, Undefined_and_reserved_two_byte_slots)
{
    /* Cells the two-byte map marks invalid in long mode. */
    static const uint8_t undefined[] = {
        0x04, 0x0A, 0x0C, 0x0E, 0x0F, 0x24, 0x25, 0x26, 0x27, 0x36, 0x39,
        0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x6C, 0x6D, 0x7A, 0x7B, 0xA6, 0xA7
    };
    for (const uint8_t second : undefined) {
        EXPECT_EQ(x64({ 0x0F, second, 0xC0 }), ERR_UNDEFINED);
    }
    EXPECT_EQ(x64({ 0x0F, 0xFF, 0xC0 }), ERR_UNDEFINED);

    /* Their defined neighbours, so the list above is not an off-by-one. */
    EXPECT_EQ(x64({ 0x0F, 0x05 }), 2u);              /* SYSCALL */
    EXPECT_EQ(x64({ 0x0F, 0x06 }), 2u);              /* CLTS */
    EXPECT_EQ(x64({ 0x0F, 0x07 }), 2u);              /* SYSRET */
    EXPECT_EQ(x64({ 0x0F, 0x0B }), 2u);              /* UD2 */
    EXPECT_EQ(x64({ 0x0F, 0x20, 0xC0 }), 3u);        /* MOV CR, r64 */
    EXPECT_EQ(x64({ 0x0F, 0x30 }), 2u);              /* WRMSR */
    EXPECT_EQ(x64({ 0x0F, 0x37 }), 2u);              /* GETSEC */
    EXPECT_EQ(x64({ 0x0F, 0xA6, 0xC0 }), ERR_UNDEFINED);

    EXPECT_EQ(x64({ 0x0F, 0xF7, 0xC0 }), 3u); /* MASKMOVQ: map says three bytes, no disp */
    EXPECT_EQ(x64({ 0x0F, 0xF7, 0x05, 1, 2, 3, 4 }), 3u); /* even with a memory-looking ModRM */
}

TEST(TwoByteOpcodeTest, Prefixes_stack_onto_two_byte_families)
{
    EXPECT_EQ(x64({ 0x66, 0x0F, 0x10, 0x00 }), 4u); /* MOVUPD */
    EXPECT_EQ(x64({ 0xF3, 0x0F, 0x10, 0x00 }), 4u); /* MOVSS */
    EXPECT_EQ(x64({ 0xF2, 0x0F, 0x10, 0x00 }), 4u); /* MOVSD */
    EXPECT_EQ(x64({ 0x64, 0x0F, 0x10, 0x00 }), 4u); /* GS MOVUPS xmm, [rax] */
    EXPECT_EQ(x64({ 0x48, 0x0F, 0x10, 0x00 }), 4u); /* REX.W MOVUPS */
    EXPECT_EQ(x64({ 0x66, 0xF3, 0x0F, 0x10, 0x00 }), 5u);
    EXPECT_EQ(x64({ 0xF3, 0xF2, 0x0F, 0x10, 0x00 }), ERR_UNDEFINED); /* two rep prefixes */
}
