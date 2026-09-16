// test_x87_opcodes.cpp
// x87 escape coverage (0xD8-0xDF) in both modes.
//
// Two claims are checked, and each is only made where the encoding rules are
// unambiguous:
//
//   1. Memory operands (mod != 3) go through the ordinary ModRM/SIB/displacement
//      rules, so their lengths come from the independent model - including the
//      ones the old decoder got wrong by ignoring a displacement.
//   2. Register operands (mod == 3) are always two bytes when the encoding is
//      accepted, and every other answer must be an error code - never a read
//      beyond the buffer.
//
// Plus the handful of register forms whose names are certain (FLD1, FST, FCHS,
// FNSTSW AX, ...).
//
// Suites: X87OpcodeTest (x64), X86x87Test (IA-32).

#include <cstdint>
#include <vector>

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

namespace {

/* Extensions the x87 escape leaves undefined even in their memory form:
 * 0xD9 /1 (reserved), 0xDB /4 and 0xDB /6 (deprecated). */
bool reserved_extension(uint8_t opc, uint8_t modrm)
{
    const unsigned reg = (modrm >> 3) & 7u;
    if (modrm >> 6 == 3u) {
        return false; /* the register-form sweep asserts those separately */
    }
    return (opc == 0xD9 && reg == 1u) || (opc == 0xDB && (reg == 4u || reg == 6u));
}

template <typename Decode>
void check_memory_forms(bool long_mode, Decode decode, size_t& failures, size_t& combos)
{
    static const uint8_t modrms[] = { 0x00, 0x05, 0x04, 0x24, 0x44, 0x84, 0x40, 0x80, 0x35 };

    for (const uint8_t opc : { 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF }) {
        for (const bool has_67 : { false, true }) {
            for (const uint8_t modrm : modrms) {
                for (const uint8_t sib : { 0x00, 0x25, 0x05 }) {
                    ++combos;
                    /* 0x67 selects 32-bit addressing in long mode (same widths) and
                     * 16-bit addressing in IA-32 (no SIB, disp16). */
                    const bool model16 = has_67 && !long_mode;
                    std::vector<uint8_t> b;
                    if (has_67) {
                        b.push_back(0x67);
                    }
                    b.push_back(opc);
                    b.push_back(modrm);
                    const bool uses_sib = !model16 && (modrm >> 6) != 3u && (modrm & 7u) == 4u;
                    if (uses_sib) {
                        b.push_back(sib);
                    }
                    const size_t span = lzmodel::modrm_span(modrm, sib, false, model16);
                    const size_t header = uses_sib ? 2u : 1u;
                    if (span < header) {
                        continue;
                    }
                    for (size_t i = 0; i < span - header; ++i) {
                        b.push_back(static_cast<uint8_t>(0x60 + i));
                    }
                    const size_t expected = (has_67 ? 1u : 0u) + 1u + span;
                    if (b.size() > LDZ_MAX_INSNS_LEN) {
                        continue;
                    }
                    const size_t got = decode(b.data(), b.size());
                    /* The x87 escape accepts or rejects the encoding, but a
                     * complete memory form must decode to its modelled length. */
                    if (got != expected && !(reserved_extension(opc, modrm) && got >= 0xE0)) {
                        ++failures;
                        std::printf("    opc=%02X modrm=%02X sib=%02X 67=%d expected=%zu got=%zu\n", opc,
                                    modrm, sib, has_67 ? 1 : 0, expected, got);
                    }
                }
            }
        }
    }
}

template <typename Decode>
void check_register_forms(Decode decode, size_t& failures, size_t& combos)
{
    for (const uint8_t opc : { 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF }) {
        for (uint32_t modrm = 0xC0; modrm <= 0xFF; ++modrm) {
            ++combos;
            const size_t got = decode(std::vector<uint8_t>{ opc, static_cast<uint8_t>(modrm) }.data(), 2);
            /* A register x87 form is exactly two bytes; anything else must be an
             * explicit rejection rather than a length. */
            if (got != 2u && got < 0xE0) {
                ++failures;
                std::printf("    opc=%02X modrm=%02X expected 2 or error, got %zu\n", opc, modrm, got);
            }
        }
    }
}

size_t decode64(const uint8_t* p, size_t n)
{
    return lendiza::detail::amd64traits::ldiza(p, n);
}

size_t decode32(const uint8_t* p, size_t n)
{
    return lendiza::detail::x86traits::ldiza(p, n);
}

} // namespace

TEST(X87OpcodeTest, Memory_forms_in_long_mode)
{
    size_t failures = 0;
    size_t combos = 0;
    check_memory_forms(true, decode64, failures, combos);
    std::printf("    %zu memory forms\n", combos);
    EXPECT_EQ(failures, 0u);
    EXPECT_TRUE(combos > 200u);
}

TEST(X87OpcodeTest, Register_forms_are_two_bytes_or_rejected)
{
    size_t failures = 0;
    size_t combos = 0;
    check_register_forms(decode64, failures, combos);
    std::printf("    %zu register forms\n", combos);
    EXPECT_EQ(failures, 0u);
    EXPECT_EQ(combos, 8u * 64u);
}

TEST(X87OpcodeTest, Named_register_forms)
{
    EXPECT_EQ(x64({ 0xD9, 0xE0 }), 2u); /* FCHS */
    EXPECT_EQ(x64({ 0xD9, 0xE1 }), 2u); /* FABS */
    EXPECT_EQ(x64({ 0xD9, 0xC0 }), 2u); /* FLD1 */
    EXPECT_EQ(x64({ 0xD9, 0xEE }), 2u); /* FLDZ */
    EXPECT_EQ(x64({ 0xDD, 0xD0 }), 2u); /* FST %st(0) */
    EXPECT_EQ(x64({ 0xDF, 0xE0 }), 2u); /* FNSTSW AX */
    EXPECT_EQ(x64({ 0xDE, 0xC0 }), 2u); /* FADDP %st,%st(1) */
    /* FWAIT (0x9B) is not in the legacy-prefix set lendiza/xendiza share, so it
     * stays its own one-byte instruction. */
    EXPECT_EQ(x64({ 0x9B, 0xD9, 0xE0 }), 1u);
}

TEST(X87OpcodeTest, Memory_operand_with_displacements)
{
    EXPECT_EQ(x64({ 0xDD, 0x00 }), 2u);                   /* FLD qword [rax] */
    EXPECT_EQ(x64({ 0xDD, 0x40, 0x08 }), 3u);             /* FLD [rax+8] */
    EXPECT_EQ(x64({ 0xDD, 0x80, 1, 2, 3, 4 }), 6u);       /* FLD [rax+disp32] */
    EXPECT_EQ(x64({ 0xDD, 0x05, 1, 2, 3, 4 }), 6u);       /* FLD [rip+disp32] */
    EXPECT_EQ(x64({ 0xDD, 0x04, 0x25, 1, 2, 3, 4 }), 7u); /* FLD [abs32] with SIB */
    EXPECT_EQ(x64({ 0xDD, 0x04 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x66, 0xDD, 0x00 }), 3u);             /* operand-size prefix counted into the length */
    EXPECT_EQ(x64({ 0x67, 0xDD, 0x00 }), 3u);             /* 32-bit addressing */
}

TEST(X87OpcodeTest, Undefined_register_encodings_are_rejected)
{
    /* D9 /D1-DF, E2, E3, E6, E7, EF have no x87 encoding. */
    EXPECT_EQ(x64({ 0xD9, 0xD1 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0xD9, 0xDF }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0xD9, 0xE2 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0xD9, 0xEF }), ERR_UNDEFINED);
    /* 0xDA /5 (E8-DF band) is a defined comparison form. */
    EXPECT_EQ(x64({ 0xDA, 0xE9 }), 2u);
    /* 0xDB memory forms: /4 and /6 are the deprecated extensions. */
    EXPECT_EQ(x64({ 0xDB, 0x20 }), ERR_UNDEFINED); /* mod=00 reg=4 */
    EXPECT_EQ(x64({ 0xDB, 0x30 }), ERR_UNDEFINED); /* mod=00 reg=6 */
    EXPECT_EQ(x64({ 0xDB, 0x28 }), 2u);           /* mod=00 reg=5 is accepted */
}

TEST(X86x87Test, Memory_forms_in_ia32)
{
    size_t failures = 0;
    size_t combos = 0;
    check_memory_forms(false, decode32, failures, combos);
    std::printf("    %zu memory forms\n", combos);
    EXPECT_EQ(failures, 0u);
}

TEST(X86x87Test, Register_forms_are_two_bytes_or_rejected)
{
    size_t failures = 0;
    size_t combos = 0;
    check_register_forms(decode32, failures, combos);
    EXPECT_EQ(failures, 0u);
    EXPECT_EQ(combos, 8u * 64u);
}

TEST(X86x87Test, Sixteen_bit_addressing_still_shapes_the_operand)
{
    EXPECT_EQ(x86({ 0xDD, 0x00 }), 2u);              /* FLD qword [eax] */
    EXPECT_EQ(x86({ 0x67, 0xDD, 0x00 }), 3u);        /* 16-bit: [BX] - no disp */
    EXPECT_EQ(x86({ 0x67, 0xDD, 0x06, 1, 2 }), 5u);  /* 16-bit: [disp16] */
    EXPECT_EQ(x86({ 0x67, 0xDD, 0x80, 1, 2 }), 5u);  /* 16-bit: [BX]+disp16 */
    EXPECT_EQ(x86({ 0xDD, 0x05, 1, 2, 3, 4 }), 6u);  /* 32-bit: [disp32] */
    EXPECT_EQ(x86({ 0xDD, 0x85, 1, 2, 3, 4 }), 6u);  /* 32-bit: [edi]+disp32 */
    EXPECT_EQ(x86({ 0xDF, 0xE0 }), 2u);              /* FNSTSW AX */
}

TEST(X87OpcodeTest, Escapes_do_not_read_a_byte_that_is_not_there)
{
    /* Every x87 opcode truncated at one byte must ask for more, not guess. */
    for (const uint8_t opc : { 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF }) {
        EXPECT_EQ(x64({ opc }), ERR_INSUFFICIENT);
        EXPECT_EQ(x86({ opc }), ERR_INSUFFICIENT);
        EXPECT_EQ(x64({ 0x67, opc }), ERR_INSUFFICIENT);
    }
}
