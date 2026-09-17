// test_basic_instructions.cpp
// One-byte opcode map coverage for x86-64: the register/memory ALU families, the
// stack and control-transfer forms, and the immediate-bearing short forms.
//
// Expected lengths come from lzmodel::modrm_span(), an independent statement of
// the Intel ModRM/SIB/displacement rules, so this file does not restate the
// library's own tables.  For every form two things are asserted:
//
//   * the complete encoding decodes to exactly its length, and
//   * dropping the last byte makes it INSUFFICIENT rather than shorter.
//
// The second half is the property that matters in a driver: a decoder that
// guesses a length from a truncated window silently desynchronises a linear scan.
//
// Suite: DisasmTest.

#include <cstdint>
#include <vector>

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

namespace {

/* Assembles [prefixes?] [opcode] [ModRM] [SIB] [disp] [imm] from the model. */
std::vector<uint8_t> build(bool rex_b, bool addr16, uint8_t opc, uint8_t modrm, uint8_t sib,
                           size_t imm_bytes, bool drop_last)
{
    std::vector<uint8_t> v;
    size_t prefix = 0;
    if (rex_b) {
        v.push_back(0x41); /* REX.B */
        ++prefix;
    }
    if (addr16) {
        v.push_back(0x67);
        ++prefix;
    }
    v.push_back(opc);
    v.push_back(modrm);

    const unsigned mod = modrm >> 6;
    const bool uses_sib = !addr16 && mod != 3u && (modrm & 7u) == 4u;
    if (uses_sib) {
        v.push_back(sib);
    }

    const size_t span = lzmodel::modrm_span(modrm, sib, rex_b, addr16);
    const size_t disp = span - (uses_sib ? 2u : 1u);
    for (size_t i = 0; i < disp; ++i) {
        v.push_back(static_cast<uint8_t>(0x20 + i)); /* arbitrary disp bytes */
    }
    for (size_t i = 0; i < imm_bytes; ++i) {
        v.push_back(static_cast<uint8_t>(0x70 + i));
    }

    if (drop_last) {
        v.pop_back();
    }
    return v;
}

/* Decodes every ModRM form for one opcode, with and without a trailing
 * immediate, and with a SIB byte where the ModRM calls for one. */
template <typename Decode>
void check_modrm_family(uint8_t opc, size_t imm_bytes, Decode decode, size_t& checks,
                        size_t& failures)
{
    static const uint8_t mods[] = { 0x00, 0x05, 0x3D, 0x04, 0x24, 0x44, 0x84, 0x40, 0x7D, 0x80, 0xBD,
                                    0xC0, 0xFF };
    static const uint8_t sibs[] = { 0x00, 0x05, 0x25, 0x0D, 0xB5, 0xFD };

    for (const uint8_t modrm : mods) {
        for (const bool rex_b : { false, true }) {
            const uint8_t sib = (modrm & 7u) == 4u ? 0x25 : 0x00; /* base=101 */
            const size_t prefix = rex_b ? 1u : 0u;

            if (lzmodel::requires_memory_operand(opc) && (modrm >> 6) == 3u) {
                /* Complete encoding of an instruction the CPU cannot execute at
                 * all, so the answer is the error, not a length.  The truncation
                 * check below is about a missing operand byte and does not apply. */
                std::vector<uint8_t> invalid = build(rex_b, false, opc, modrm, sib, imm_bytes, false);
                ++checks;
                if (decode(invalid.data(), invalid.size()) != ERR_UNDEFINED) {
                    ++failures;
                    std::printf("    opc=%02X modrm=%02X must be undefined, got %zu\n", opc, modrm,
                                decode(invalid.data(), invalid.size()));
                }
                continue;
            }

            const size_t expected =
                prefix + 1 + lzmodel::modrm_span(modrm, sib, rex_b, false) + imm_bytes;

            std::vector<uint8_t> full = build(rex_b, false, opc, modrm, sib, imm_bytes, false);
            if (full.size() > LDZ_MAX_INSNS_LEN) {
                continue;
            }
            ++checks;
            if (decode(full.data(), full.size()) != expected) {
                ++failures;
                std::printf("    opc=%02X modrm=%02X sib=%02X rexb=%d expected=%zu\n", opc, modrm, sib,
                            rex_b ? 1 : 0, expected);
            }

            if (full.size() >= 2) {
                std::vector<uint8_t> cut = build(rex_b, false, opc, modrm, sib, imm_bytes, true);
                ++checks;
                if (decode(cut.data(), cut.size()) != ERR_INSUFFICIENT) {
                    ++failures;
                    std::printf("    truncated opc=%02X modrm=%02X n=%zu -> %zu\n", opc, modrm,
                                cut.size(), decode(cut.data(), cut.size()));
                }
            }
        }
    }

    /* Every SIB base/index combination for one ModRM that forces a SIB byte. */
    for (const uint8_t sib : sibs) {
        const size_t expected = 1 + lzmodel::modrm_span(0x04, sib, false, false) + imm_bytes;
        std::vector<uint8_t> full = build(false, false, opc, 0x04, sib, imm_bytes, false);
        ++checks;
        if (decode(full.data(), full.size()) != expected) {
            ++failures;
            std::printf("    opc=%02X sib=%02X expected=%zu got=%zu\n", opc, sib, expected,
                        decode(full.data(), full.size()));
        }
    }
}

size_t decode64(const uint8_t* p, size_t n)
{
    return lendiza::detail::amd64traits::ldiza(p, n);
}

} // namespace

TEST(DisasmTest, ALU_Eb_Gb_forms_0x00_to_0x03)
{
    size_t checks = 0;
    size_t failures = 0;
    for (const uint8_t opc : { 0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x28, 0x29, 0x38, 0x39 }) {
        check_modrm_family(opc, 0, &decode64, checks, failures);
    }
    std::printf("    %zu checks\n", checks);
    EXPECT_EQ(failures, 0u);
}

TEST(DisasmTest, Mov_0x88_to_0x8B_and_lea_0x8D)
{
    size_t checks = 0;
    size_t failures = 0;
    for (const uint8_t opc : { 0x88, 0x89, 0x8A, 0x8B, 0x8D }) {
        check_modrm_family(opc, 0, &decode64, checks, failures);
    }
    EXPECT_EQ(failures, 0u);
}

TEST(DisasmTest, Lea_0x8D_rejects_a_register_operand)
{
    /* The CPU raises EXCEPTION_ILLEGAL_INSTRUCTION for the whole mod=11 band of
     * 8D regardless of what any register holds, so there is no length here. */
    EXPECT_EQ(x64({ 0x8D, 0xC0 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x8D, 0xC5 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x8D, 0xFF }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x48, 0x8D, 0xC0 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x67, 0x8D, 0xFF }), ERR_UNDEFINED);
    EXPECT_EQ(x86({ 0x8D, 0xC0 }), ERR_UNDEFINED);
    EXPECT_EQ(x86({ 0x67, 0x8D, 0xC0 }), ERR_UNDEFINED);

    /* Where the rule stops: a memory operand is fine, and MOV has no such
     * requirement. */
    EXPECT_EQ(x64({ 0x8D, 0x00 }), 2u);                              // LEA eax, [eax]
    EXPECT_EQ(x64({ 0x8D, 0x04, 0x25, 0, 0, 0, 0 }), 7u);            // LEA eax, [disp32]
    EXPECT_EQ(x64({ 0x8D, 0x44, 0x25, 0x10 }), 4u);                  // LEA eax, [..+disp8]
    EXPECT_EQ(x64({ 0x8B, 0xC0 }), 2u);                              // MOV eax, eax
    EXPECT_EQ(x86({ 0x8D, 0x00 }), 2u);
}

TEST(DisasmTest, Segment_moves_0x8C_0x8E)
{
    size_t checks = 0;
    size_t failures = 0;
    for (const uint8_t opc : { 0x8C, 0x8E }) {
        check_modrm_family(opc, 0, &decode64, checks, failures);
    }
    EXPECT_EQ(failures, 0u);
}

TEST(DisasmTest, Xchg_test_and_pop_forms)
{
    size_t checks = 0;
    size_t failures = 0
        ;
    check_modrm_family(0x85, 0, &decode64, checks, failures); // TEST Ev, Gv
    check_modrm_family(0x87, 0, &decode64, checks, failures); // XCHG Ev, Gv
    check_modrm_family(0x8F, 0, &decode64, checks, failures); // POP Ev
    check_modrm_family(0x63, 0, &decode64, checks, failures); // MOVSXD Gv, Ev
    EXPECT_EQ(failures, 0u);
}

TEST(DisasmTest, Scas_and_imul_are_not_confused)
{
    /* 0xAE/0xAF are SCAS m8 / SCAS m16-32-64: single-byte string operations.
     * The two-operand IMUL lives at 0F AF, so no ModRM may be expected here. */
    EXPECT_EQ(x64({ 0xAE, 0xC0 }), 1u);
    EXPECT_EQ(x64({ 0xAF, 0xC0 }), 1u);
    EXPECT_EQ(x64({ 0x0F, 0xAF, 0xC1 }), 3u); /* IMUL eax, ecx */

    /* The IA-32 two-byte map used to mark this row as a six-byte form; both modes
     * now agree that IMUL carries a ModRM only. */
    EXPECT_EQ(x86({ 0x0F, 0xAF, 0xC1 }), 3u);
}

TEST(DisasmTest, Push_and_pop_register_short_forms_are_one_byte)
{
    for (uint8_t opc = 0x50; opc <= 0x5F; ++opc) {
        EXPECT_EQ(x64({ opc }), 1u);
        EXPECT_EQ(x64({ 0x48, opc }), 2u); /* REX counts into the same instruction */
    }
}

TEST(DisasmTest, Jcc_rel8_is_two_bytes)
{
    for (uint8_t opc = 0x70; opc <= 0x7F; ++opc) {
        EXPECT_EQ(x64({ opc, 0x12 }), 2u);
        EXPECT_EQ(x64({ opc }), ERR_INSUFFICIENT);
        EXPECT_EQ(x64({ 0x66, opc, 0x12 }), 3u);
    }
}

TEST(DisasmTest, Accumulator_immediates)
{
    static const uint8_t ib_ops[] = { 0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C };
    for (const uint8_t opc : ib_ops) {
        EXPECT_EQ(x64({ opc, 0x7F }), 2u);           /* ADD AL, imm8 - width independent */
        EXPECT_EQ(x64({ 0x66, opc, 0x7F }), 3u);
        EXPECT_EQ(x64({ opc }), ERR_INSUFFICIENT);
    }

    static const uint8_t iz_ops[] = { 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D };
    for (const uint8_t opc : iz_ops) {
        EXPECT_EQ(x64({ opc, 1, 2, 3, 4 }), 5u);     /* ADD eAX, imm32 */
        EXPECT_EQ(x64({ 0x66, opc, 1, 2 }), 4u);     /* ADD ax, imm16 */
        EXPECT_EQ(x64({ opc, 1, 2, 3 }), ERR_INSUFFICIENT);
    }
}

TEST(DisasmTest, Mov_immediate_short_forms)
{
    for (uint8_t opc = 0xB0; opc <= 0xB7; ++opc) {
        EXPECT_EQ(x64({ opc, 0x7F }), 2u);           /* MOV r8, imm8 */
        EXPECT_EQ(x64({ 0x48, opc, 0x7F }), 3u);     /* REX extends the register, not the imm */
    }
    for (uint8_t opc = 0xB8; opc <= 0xBF; ++opc) {
        EXPECT_EQ(x64({ opc, 1, 2, 3, 4 }), 5u);                     /* MOV r32, imm32 */
        EXPECT_EQ(x64({ 0x48, opc, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u);  /* MOV r64, imm64 */
        EXPECT_EQ(x64({ 0x66, opc, 1, 2 }), 4u);                     /* MOV r16, imm16 */
    }
}

TEST(DisasmTest, Fixed_length_one_byte_ops)
{
    static const uint8_t single[] = { 0x90, 0x91, 0x97, 0x98, 0x99, 0x9C, 0x9D, 0x9E, 0x9F, 0xA6,
                                      0xA7, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xC3, 0xC9, 0xCC,
                                      0xF1, 0xF4, 0xF5, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD };
    for (const uint8_t opc : single) {
        EXPECT_EQ(x64({ opc }), 1u);
        EXPECT_EQ(x64({ 0x66, opc }), 2u);
        EXPECT_EQ(x64({ 0x67, 0x64, opc }), 3u);
    }
}

TEST(DisasmTest, Near_control_and_stack_forms)
{
    EXPECT_EQ(x64({ 0xC2, 0x08, 0x00 }), 3u);   /* RET imm16 */
    EXPECT_EQ(x64({ 0xC8, 0x00, 0x00, 0x02 }), 4u); /* ENTER imm16, imm8 */
    EXPECT_EQ(x64({ 0xE8, 1, 2, 3, 4 }), 5u);   /* CALL rel32 */
    EXPECT_EQ(x64({ 0xE9, 1, 2, 3, 4 }), 5u);   /* JMP rel32 */
    EXPECT_EQ(x64({ 0xEB, 0x10 }), 2u);         /* JMP rel8 */
    EXPECT_EQ(x64({ 0x68, 1, 2, 3, 4 }), 5u);   /* PUSH imm32 */
    EXPECT_EQ(x64({ 0x6A, 0x10 }), 2u);         /* PUSH imm8 */
    EXPECT_EQ(x64({ 0xCD, 0x20 }), 2u);         /* INT imm8 */
}

TEST(DisasmTest, Moffs_forms_follow_the_address_size)
{
    for (const uint8_t opc : { 0xA0, 0xA1, 0xA2, 0xA3 }) {
        EXPECT_EQ(x64({ opc, 1, 2, 3, 4, 5, 6, 7, 8 }), 9u);   /* moffs64 */
        EXPECT_EQ(x64({ 0x67, opc, 1, 2, 3, 4 }), 6u);         /* moffs32 */
        EXPECT_EQ(x64({ opc, 1, 2, 3, 4, 5, 6, 7 }), ERR_INSUFFICIENT);
    }
}

TEST(DisasmTest, String_and_vector_moves_are_single_bytes)
{
    static const uint8_t str_ops[] = { 0xA4, 0xA5, 0xA6, 0xA7, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF };
    for (const uint8_t opc : str_ops) {
        EXPECT_EQ(x64({ opc }), 1u);
        EXPECT_EQ(x64({ 0xF3, opc }), 2u); /* REP/REPE prefix */
        EXPECT_EQ(x64({ 0xF2, opc }), 2u);
    }
}
