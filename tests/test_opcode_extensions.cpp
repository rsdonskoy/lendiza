// test_opcode_extensions.cpp
// Opcode-extension groups for x86-64: the forms where the reg field of ModRM
// decides whether an immediate follows (Groups 1-6, plus IMUL's two immediate
// forms).
//
// Two things are asserted for every (opcode, ModRM, reg) triple:
//   * the total length, built from the independent ModRM model plus the
//     immediate width the group rule prescribes, and
//   * that removing the final byte yields INSUFFICIENT rather than a shorter
//     answer - the property a driver relies on when it decodes a truncated window.
//
// The imm table below is written out per Intel group rule, not read from the
// library.
//
// Suite: OpcodeExtTest.

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

/* Immediate bytes that follow ModRM+SIB+disp for one group opcode, or
 * not exist. */
size_t group_imm(uint8_t opc, uint8_t modrm, bool has_66, bool rex_w, size_t& err)
{
    err = 0;
    const unsigned reg = (modrm >> 3) & 7u;
    const unsigned mod = modrm >> 6;

    switch (opc) {
    case 0x80:
    case 0x82:
    case 0x83:
    case 0xC0:
    case 0xC1:
        return 1; /* Group 1 / Group 2: imm8 */
    case 0x81:
        return has_66 ? 2u : 4u;                       /* Group 1: Iz */
    case 0xD0:
    case 0xD1:
    case 0xD2:
    case 0xD3:
        return 0;                                      /* Group 3: no immediate */
    case 0xC6:
        if (reg == 0u) {
            return 1;                                  /* Group 4: MOV r/m8, imm8 */
        }
        if (reg == 7u && mod == 3u) {
            return 1;
        }
        err = ERR_UNDEFINED;
        return 0;
    case 0xC7:
        if (reg == 0u) {
            return has_66 ? 2u : 4u;                   /* Group 4: MOV r/m, Iz */
        }
        if (reg == 7u && mod == 3u) {
            return 2;
        }
        err = ERR_UNDEFINED;
        return 0;
    case 0xF6:
        return (reg == 0u || reg == 1u) ? 1u : 0u;     /* Group 5: TEST/NOT r/m8, imm8 */
    case 0xF7:
        return (reg == 0u || reg == 1u) ? (has_66 ? 2u : 4u) : 0u;
    case 0xFE:
        if (reg == 0u || reg == 1u) {
            return 0;                                  /* INC/DEC r/m8 */
        }
        err = ERR_UNDEFINED;
        return 0;
    case 0xFF:
        if (reg == 7u) {
            err = ERR_UNDEFINED;                       /* invalid extension */
            return 0;
        }
        return 0;
    case 0x69:
        return has_66 ? 2u : 4u;                       /* IMUL Gv, Ev, Iz */
    case 0x6B:
        return 1;                                      /* IMUL Gv, Ev, Ib */
    default:
        err = ERR_UNKNOWN;
        return 0;
    }
}

struct Prefix {
    uint8_t byte;
    bool rex_w;
    bool has_66;
};

/* Walks one group opcode over ModRM forms and prefix variants. */
void check_group(uint8_t opc, size_t& failures)
{
    size_t combos = 0;
    static const uint8_t mods[] = { 0x00, 0x05, 0x04, 0x24, 0x44, 0x84, 0x40, 0x7D, 0x80, 0xBD, 0xC0,
                                    0xC8, 0xC9, 0xFF, 0xF8, 0xFE };
    static const Prefix prefixes[] = { { 0x00, false, false }, { 0x48, true, false },
                                       { 0x66, false, true },  { 0x67, false, false } };

    for (const Prefix& p : prefixes) {
        for (const uint8_t modrm : mods) {
            for (unsigned reg = 0; reg < 8; ++reg) {
                const uint8_t mr = static_cast<uint8_t>((modrm & 0xC7u) | (reg << 3));
                ++combos;
                size_t err = 0;
                const size_t imm = group_imm(opc, mr, p.has_66, p.rex_w, err);

                const unsigned mod = mr >> 6;
                /* 0x67 in long mode selects 32-bit addressing, whose ModRM/SIB and
                 * displacement widths match the default - only moffs shrinks, and
                 * no group opcode here uses moffs. */
                const bool addr16 = false;
                const bool uses_sib = !addr16 && mod != 3u && (mr & 7u) == 4u;
                const uint8_t sib = uses_sib ? 0x25 : 0x00;
                const size_t span = lzmodel::modrm_span(mr, sib, false, addr16);
                const size_t header = uses_sib ? 2u : 1u;
                if (span < header) {
                    continue; /* model disagrees with the form: skip, never underflow */
                }

                std::vector<uint8_t> v;
                if (p.byte != 0x00) {
                    v.push_back(p.byte);
                }
                v.push_back(opc);
                v.push_back(mr);
                if (uses_sib) {
                    v.push_back(sib);
                }
                const size_t disp = span - (uses_sib ? 2u : 1u);
                for (size_t i = 0; i < disp; ++i) {
                    v.push_back(static_cast<uint8_t>(0x40 + i));
                }
                for (size_t i = 0; i < imm; ++i) {
                    v.push_back(static_cast<uint8_t>(0x90 + i));
                }
                if (v.size() > LDZ_MAX_INSNS_LEN) {
                    continue;
                }

                const size_t got = decode64(v.data(), v.size());
                if (err != 0) {
                    if (got < 0xE0) {
                        ++failures;
                        std::printf("    opc=%02X modrm=%02X expected error, got %zu\n", opc, mr, got);
                    }
                    continue;
                }
                if (got != v.size()) {
                    ++failures;
                    std::printf("    opc=%02X modrm=%02X pfx=%02X expected=%zu got=%zu\n", opc, mr,
                                p.byte, v.size(), got);
                }

                if (v.size() >= 2) {
                    std::vector<uint8_t> cut(v.begin(), v.end() - 1);
                    const size_t got_cut = decode64(cut.data(), cut.size());
                    if (got_cut != ERR_INSUFFICIENT) {
                        ++failures;
                        std::printf("    truncated opc=%02X modrm=%02X -> %zu\n", opc, mr, got_cut);
                    }
                }
            }
        }
    }
    std::printf("    opc=%02X: %zu combinations\n", opc, combos);
}

} // namespace

TEST(OpcodeExtTest, Group1_0x80_to_0x83)
{
    size_t failures = 0;
    for (const uint8_t opc : { 0x80, 0x81, 0x83 }) {
        check_group(opc, failures);
    }
    EXPECT_EQ(failures, 0u);
}

TEST(OpcodeExtTest, Group2_and_3_shifts_0xC0_to_0xD3)
{
    size_t failures = 0;
    for (const uint8_t opc : { 0xC0, 0xC1, 0xD0, 0xD1, 0xD2, 0xD3 }) {
        check_group(opc, failures);
    }
    EXPECT_EQ(failures, 0u);
}

TEST(OpcodeExtTest, Group4_mov_immediate_0xC6_0xC7)
{
    size_t failures = 0;
    check_group(0xC6, failures);
    check_group(0xC7, failures);
    EXPECT_EQ(failures, 0u);
}

TEST(OpcodeExtTest, Group5_unary_0xF6_0xF7)
{
    size_t failures = 0;
    check_group(0xF6, failures);
    check_group(0xF7, failures);
    EXPECT_EQ(failures, 0u);
}

TEST(OpcodeExtTest, Group6_inc_dec_call_jmp_0xFE_0xFF)
{
    size_t failures = 0;
    check_group(0xFE, failures);
    check_group(0xFF, failures);
    EXPECT_EQ(failures, 0u);
}

TEST(OpcodeExtTest, Imul_immediate_forms_0x69_0x6B)
{
    size_t failures = 0;
    check_group(0x69, failures);
    check_group(0x6B, failures);
    EXPECT_EQ(failures, 0u);
}

TEST(OpcodeExtTest, Group_extension_specific_cases)
{
    /* F6 /0 tests with imm8, F6 /2 not: TEST byte [eax], imm8 vs NOT byte [eax] */
    EXPECT_EQ(x64({ 0xF6, 0x00, 0x11 }), 3u);
    EXPECT_EQ(x64({ 0xF6, 0x10 }), 2u);
    EXPECT_EQ(x64({ 0xF7, 0x00, 1, 2, 3, 4 }), 6u);
    EXPECT_EQ(x64({ 0xF7, 0x10 }), 2u);
    EXPECT_EQ(x64({ 0xFE, 0x00 }), 2u);                 /* INC byte [rax] */
    EXPECT_EQ(x64({ 0xFE, 0x38 }), ERR_UNDEFINED);      /* /7 is not defined */
    EXPECT_EQ(x64({ 0xFF, 0x10 }), 2u);                 /* CALL/Q/JMP near [rax] */
    EXPECT_EQ(x64({ 0xFF, 0x38 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0xC6, 0x00, 0x11 }), 3u);           /* MOV byte [rax], imm8 */
    EXPECT_EQ(x64({ 0xC6, 0x08 }), ERR_UNDEFINED);      /* /1 undefined */
    EXPECT_EQ(x64({ 0xC7, 0x00, 1, 2, 3, 4 }), 6u);     /* MOV r/m32, imm32 */
    EXPECT_EQ(x64({ 0x66, 0xC7, 0x00, 1, 2 }), 5u);     /* MOV r/m16, imm16 */
    EXPECT_EQ(x64({ 0x48, 0xC7, 0x00, 1, 2, 3, 4 }), 7u); /* REX.W keeps imm32 */
    EXPECT_EQ(x64({ 0x48, 0x81, 0xC0, 1, 2, 3, 4 }), 7u); /* ADD rax, imm32 */
}
