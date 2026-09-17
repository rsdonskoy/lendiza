// test_x86_basic.cpp
// One-byte opcode map coverage for IA-32 (32-bit) mode.
//
// Same shape as test_basic_instructions.cpp, with the differences that matter
// between the two modes: 0x40-0x4F are INC/DEC rather than REX, the far
// CALL/JMP forms exist, PUSHAD/POPAD and the segment PUSH/POP forms are legal,
// and 0x67 switches the ModRM model to 16-bit addressing (no SIB, disp16).
//
// Expected lengths come from lzmodel::modrm_span(), not from the library's
// tables; each form is also decoded one byte short to confirm the decoder
// refuses to guess.
//
// Suite: X86BasicTest.

#include <cstdint>
#include <vector>

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

namespace {

size_t decode32(const uint8_t* p, size_t n)
{
    return lendiza::detail::x86traits::ldiza(p, n);
}

/* [prefixes?] opcode ModRM [SIB] [disp] [imm], assembled from the model. */
std::vector<uint8_t> assemble(bool addr16, uint8_t opc, uint8_t modrm, uint8_t sib, size_t imm_bytes,
                              bool drop_last)
{
    std::vector<uint8_t> v;
    if (addr16) {
        v.push_back(0x67);
    }
    v.push_back(opc);
    v.push_back(modrm);

    const unsigned mod = modrm >> 6;
    const bool uses_sib = !addr16 && mod != 3u && (modrm & 7u) == 4u;
    if (uses_sib) {
        v.push_back(sib);
    }
    const size_t span = lzmodel::modrm_span(modrm, sib, false, addr16);
    const size_t disp = span - (uses_sib ? 2u : 1u);
    for (size_t i = 0; i < disp; ++i) {
        v.push_back(static_cast<uint8_t>(0x30 + i));
    }
    for (size_t i = 0; i < imm_bytes; ++i) {
        v.push_back(static_cast<uint8_t>(0x80 + i));
    }
    if (drop_last) {
        v.pop_back();
    }
    return v;
}

void check32(uint8_t opc, size_t imm_bytes, size_t& failures)
{
    static const uint8_t mods[] = { 0x00, 0x05, 0x0E, 0x04, 0x44, 0x84, 0x40, 0x80, 0xC0, 0xFF };
    for (const bool addr16 : { false, true }) {
        for (const uint8_t modrm : mods) {
            const uint8_t sib = (modrm & 7u) == 4u ? 0x25 : 0x00;

            if (lzmodel::requires_memory_operand(opc) && (modrm >> 6) == 3u) {
                /* The CPU rejects this encoding whatever the registers hold, so
                 * there is no length to report and no truncation case to test. */
                const std::vector<uint8_t> invalid =
                    assemble(addr16, opc, modrm, sib, imm_bytes, false);
                if (decode32(invalid.data(), invalid.size()) != ERR_UNDEFINED) {
                    ++failures;
                    std::printf("    opc=%02X modrm=%02X must be undefined, got %zu\n", opc, modrm,
                                decode32(invalid.data(), invalid.size()));
                }
                continue;
            }

            const size_t expected = (addr16 ? 1u : 0u) + 1u +
                                    lzmodel::modrm_span(modrm, sib, false, addr16) + imm_bytes;
            std::vector<uint8_t> full = assemble(addr16, opc, modrm, sib, imm_bytes, false);
            if (full.size() > LDZ_MAX_INSNS_LEN) {
                continue;
            }
            if (decode32(full.data(), full.size()) != expected) {
                ++failures;
                std::printf("    opc=%02X modrm=%02X addr16=%d expected=%zu got=%zu\n", opc, modrm,
                            addr16 ? 1 : 0, expected, decode32(full.data(), full.size()));
            }
            std::vector<uint8_t> cut = assemble(addr16, opc, modrm, sib, imm_bytes, true);
            if (decode32(cut.data(), cut.size()) != ERR_INSUFFICIENT) {
                ++failures;
                std::printf("    truncated opc=%02X modrm=%02X n=%zu -> %zu\n", opc, modrm, cut.size(),
                            decode32(cut.data(), cut.size()));
            }
        }
    }
}

} // namespace

TEST(X86BasicTest, ALU_and_mov_ModRM_families)
{
    size_t failures = 0;
    for (const uint8_t opc : { 0x00, 0x01, 0x02, 0x03, 0x08, 0x28, 0x38, 0x88, 0x89, 0x8B, 0x8D }) {
        check32(opc, 0, failures);
    }
    EXPECT_EQ(failures, 0u);
}

TEST(X86BasicTest, Inc_dec_short_forms_are_not_prefixes)
{
    for (uint8_t opc = 0x40; opc <= 0x4F; ++opc) {
        EXPECT_EQ(x86({ opc }), 1u);
        EXPECT_EQ(x86({ 0x66, opc }), 2u); /* r16 form, same length plus the prefix */
        EXPECT_EQ(x86({ opc, 0x90 }), 1u); /* the NOP is a separate instruction */
    }
}

TEST(X86BasicTest, Push_pop_and_stack_control)
{
    for (uint8_t opc = 0x50; opc <= 0x5F; ++opc) {
        EXPECT_EQ(x86({ opc }), 1u);
    }
    EXPECT_EQ(x86({ 0x60 }), 1u);                        /* PUSHAD */
    EXPECT_EQ(x86({ 0x61 }), 1u);                        /* POPAD */
    EXPECT_EQ(x86({ 0x06 }), 1u);                        /* PUSH ES */
    EXPECT_EQ(x86({ 0x07 }), 1u);                        /* POP ES */
    EXPECT_EQ(x86({ 0x1E }), 1u);                        /* PUSH DS */
    EXPECT_EQ(x86({ 0x9C }), 1u);                        /* PUSHFD */
    EXPECT_EQ(x86({ 0xC8, 0x00, 0x00, 0x02 }), 4u);      /* ENTER imm16, imm8 */
    EXPECT_EQ(x86({ 0xC2, 0x08, 0x00 }), 3u);            /* RET imm16 */
    EXPECT_EQ(x86({ 0xCA, 0x08, 0x00 }), 3u);            /* RETF imm16 */
}

TEST(X86BasicTest, Far_transfers_follow_the_operand_size)
{
    EXPECT_EQ(x86({ 0xEA, 1, 2, 3, 4, 5, 6 }), 7u);      /* JMP ptr16:32 */
    EXPECT_EQ(x86({ 0x9A, 1, 2, 3, 4, 5, 6 }), 7u);      /* CALL ptr16:32 */
    EXPECT_EQ(x86({ 0x66, 0xEA, 1, 2, 3, 4 }), 6u);      /* JMP ptr16:16 */
    EXPECT_EQ(x86({ 0x66, 0x9A, 1, 2, 3, 4 }), 6u);      /* CALL ptr16:16 */
    EXPECT_EQ(x86({ 0xEA, 1, 2, 3, 4, 5 }), ERR_INSUFFICIENT);
}

TEST(X86BasicTest, Bcd_and_arithmetic_adjusts_exist_here)
{
    static const uint8_t bcd[] = { 0x27, 0x2F, 0x37, 0x3F, 0xD4, 0xD5, 0xD6, 0xD7 };
    for (const uint8_t opc : bcd) {
        /* AAM/AAD carry an imm8; D6 (salc) and D7 (XLAT) are single bytes. */
        const size_t expected = (opc == 0xD4 || opc == 0xD5) ? 2u : 1u;
        EXPECT_EQ(x86({ opc, 0x10 }), expected);
    }
    /* The same bytes are undefined in long mode, which is checked in x64. */
    EXPECT_EQ(x64({ 0xD4, 0x10 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x27 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x06 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0xEA, 1, 2, 3, 4, 5, 6 }), ERR_UNDEFINED);
}

TEST(X86BasicTest, Bound_les_lds_arpl_take_ModRM)
{
    EXPECT_EQ(x86({ 0x62, 0xC1 }), 2u);      /* BOUND eax, ecx */
    EXPECT_EQ(x86({ 0x62, 0x00 }), 2u);      /* BOUND eax, [eax] */
    EXPECT_EQ(x86({ 0xC4, 0x45, 0x10 }), 3u); /* LES eax, [ebp+0x10] */
    EXPECT_EQ(x86({ 0xC5, 0x45, 0x10 }), 3u); /* LDS eax, [ebp+0x10] */
    EXPECT_EQ(x86({ 0x63, 0xC1 }), 2u);      /* ARPL cx, ax */
    EXPECT_EQ(x86({ 0x62 }), ERR_INSUFFICIENT);
}

TEST(X86BasicTest, Address_size_switches_to_16_bit_ModRM_rules)
{
    /* Same ModRM bytes, two different address sizes. */
    EXPECT_EQ(x86({ 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 7u); /* 32-bit: SIB + disp32 */
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x04 }), 3u);              /* 16-bit: [SI], no SIB byte */
    EXPECT_EQ(x86({ 0x8D, 0x06 }), 2u);                   /* 32-bit: [esi], no disp */
    EXPECT_EQ(x86({ 0x8D, 0x05, 1, 2, 3, 4 }), 6u);      /* 32-bit: [disp32] via rm=101 */
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x06, 1, 2 }), 5u);       /* 16-bit: rm=110 IS a disp16 */
    EXPECT_EQ(x86({ 0x8D, 0x80, 1, 2, 3, 4 }), 6u);        /* 32-bit: [eax+disp32] */
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x80, 1, 2 }), 5u);        /* 16-bit: [BX+disp16] */
    EXPECT_EQ(x86({ 0x8B, 0xC0 }), 2u);                             /* register form */
    EXPECT_EQ(x86({ 0x67, 0x8B, 0xC0 }), 3u);              /* ... unchanged by 0x67 */
}

TEST(X86BasicTest, Moffs_and_string_operations)
{
    for (const uint8_t opc : { 0xA0, 0xA1, 0xA2, 0xA3 }) {
        EXPECT_EQ(x86({ opc, 1, 2, 3, 4 }), 5u);      /* moffs32 */
        EXPECT_EQ(x86({ 0x67, opc, 1, 2 }), 4u);      /* moffs16 */
        EXPECT_EQ(x86({ opc, 1, 2, 3 }), ERR_INSUFFICIENT);
    }
    for (const uint8_t opc : { 0xA4, 0xA5, 0xA6, 0xA7, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE }) {
        EXPECT_EQ(x86({ opc }), 1u);
        EXPECT_EQ(x86({ 0xF3, opc }), 2u);
    }
}

TEST(X86BasicTest, Test_and_mov_immediate_short_forms)
{
    EXPECT_EQ(x86({ 0xA8, 0x7F }), 2u);                  /* TEST AL, imm8 */
    EXPECT_EQ(x86({ 0xA9, 1, 2, 3, 4 }), 5u);            /* TEST eAX, imm32 */
    EXPECT_EQ(x86({ 0x66, 0xA9, 1, 2 }), 4u);            /* TEST ax, imm16 */
    for (uint8_t opc = 0xB0; opc <= 0xB7; ++opc) {
        EXPECT_EQ(x86({ opc, 0x7F }), 2u);
    }
    for (uint8_t opc = 0xB8; opc <= 0xBF; ++opc) {
        EXPECT_EQ(x86({ opc, 1, 2, 3, 4 }), 5u);
        EXPECT_EQ(x86({ 0x66, opc, 1, 2 }), 4u);
    }
}
