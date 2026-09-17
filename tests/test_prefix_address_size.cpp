// test_prefix_address_size.cpp
// Coverage for the 0x67 address-size override in both decode modes.
//
// 0x66 and 0x67 differ in what they resize: 0x66 changes the immediate, 0x67
// changes every width derived from the addressing mode.  In long mode that means
// moffs shrinks from 8 to 4 bytes and the REX.B exception on SIB base=101 goes
// away (32-bit addressing has no RIP-relative form).  In IA-32 it means 16-bit
// addressing: no SIB byte at all, and disp16 where the 32-bit form has disp32.
//
// Suites: Prefix67Test (x64), Prefix67Test32 (IA-32).

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

// -----------------------------------------------------------------------------
// Long mode: moffs and the REX.B / SIB interaction.
// -----------------------------------------------------------------------------

TEST(Prefix67Test, Moffs_0xA1_moffs64_to_moffs32)
{
    EXPECT_EQ(x64({ 0xA1, 1, 2, 3, 4, 5, 6, 7, 8 }), 9u);    // MOV eax, moffs64
    EXPECT_EQ(x64({ 0x67, 0xA1, 1, 2, 3, 4 }), 6u);          // MOV eax, moffs32
}

TEST(Prefix67Test, Moffs_0xA0_0xA2_0xA3_all_follow_address_size)
{
    for (const uint8_t opc : { 0xA0, 0xA2, 0xA3 }) {
        EXPECT_EQ(x64({ opc, 1, 2, 3, 4, 5, 6, 7, 8 }), 9u);
        EXPECT_EQ(x64({ 0x67, opc, 1, 2, 3, 4 }), 6u);
    }
}

TEST(Prefix67Test, Moffs_with_operand_size_prefix_too)
{
    EXPECT_EQ(x64({ 0x66, 0xA1, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u); // 66 does not shrink moffs
    EXPECT_EQ(x64({ 0x66, 0x67, 0xA1, 1, 2, 3, 4 }), 7u);        // 67 does
    EXPECT_EQ(x64({ 0x67, 0x66, 0xA1, 1, 2, 3, 4 }), 7u);        // order irrelevant
}

TEST(Prefix67Test, Rip_relative_stays_disp32)
{
    EXPECT_EQ(x64({ 0x8D, 0x05, 1, 2, 3, 4 }), 6u);          // opcode + ModRM + disp32
    EXPECT_EQ(x64({ 0x67, 0x8D, 0x05, 1, 2, 3, 4 }), 7u);    // same width, plus the prefix byte
}

TEST(Prefix67Test, Sib_base5_without_REX_B_keeps_disp32)
{
    EXPECT_EQ(x64({ 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 7u);        // LEA eax, [abs32]
    EXPECT_EQ(x64({ 0x67, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 8u);  // prefix + same
}

TEST(Prefix67Test, Sib_base5_keeps_disp32_with_or_without_REX_B)
{
    /* REX.B selects a base register; it never removes the displacement.  Measured
     * on hardware, which consumes the disp32 here. */
    EXPECT_EQ(x64({ 0x49, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 8u);
    EXPECT_EQ(x64({ 0x67, 0x49, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 9u);
}

TEST(Prefix67Test, Displacements_are_not_resized_by_67)
{
    EXPECT_EQ(x64({ 0x8B, 0x00 }), 2u);                            // MOV eax, [rax]
    EXPECT_EQ(x64({ 0x67, 0x8B, 0x00 }), 3u);                      // MOV eax, [eax]
    EXPECT_EQ(x64({ 0x67, 0x8B, 0x40, 0x05 }), 4u);                // disp8
    EXPECT_EQ(x64({ 0x67, 0x8B, 0x80, 1, 2, 3, 4 }), 7u);          // disp32
    EXPECT_EQ(x64({ 0x67, 0x8D, 0x44, 0x05, 0x10 }), 5u);          // SIB + disp8
}

TEST(Prefix67Test, Immediates_and_relative_offsets_ignore_67)
{
    EXPECT_EQ(x64({ 0x67, 0x68, 1, 2, 3, 4 }), 6u);      // PUSH imm32
    EXPECT_EQ(x64({ 0x67, 0xE8, 1, 2, 3, 4 }), 6u);      // CALL rel32
    EXPECT_EQ(x64({ 0x67, 0xB8, 1, 2, 3, 4 }), 6u);      // MOV eax, imm32
    EXPECT_EQ(x64({ 0x67, 0x0F, 0xB6, 0x05, 1, 2, 3, 4 }), 8u); // MOVZX, disp32
}

TEST(Prefix67Test, Duplicate_67_counts_toward_length)
{
    EXPECT_EQ(x64({ 0x67, 0x67, 0x90 }), 3u);
}

TEST(Prefix67Test, Truncated_moffs_reports_insufficient)
{
    EXPECT_EQ(x64({ 0x67, 0xA1, 1, 2 }), ERR_INSUFFICIENT); // moffs32 needs 4 bytes
    EXPECT_EQ(x64({ 0xA1, 1, 2, 3 }), ERR_INSUFFICIENT);    // moffs64 needs 8
    EXPECT_EQ(x64({ 0x67 }), ERR_INSUFFICIENT);             // no opcode byte at all
}

// -----------------------------------------------------------------------------
// IA-32: 0x67 selects 16-bit addressing - no SIB, disp16 instead of disp32.
// -----------------------------------------------------------------------------

TEST(Prefix67Test32, Moffs_0xA1_moffs32_to_moffs16)
{
    EXPECT_EQ(x86({ 0xA1, 1, 2, 3, 4 }), 5u);    // MOV eax, moffs32
    EXPECT_EQ(x86({ 0x67, 0xA1, 1, 2 }), 4u);    // MOV eax, moffs16
}

TEST(Prefix67Test32, Mod00_rm110_takes_disp16)
{
    EXPECT_EQ(x86({ 0x8D, 0x06, 1, 2, 3, 4 }), 2u);   // LEA eax, [esi] - 32-bit rm=110 has no disp
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x06, 1, 2 }), 5u);    // 16-bit rm=110 IS a disp16
}

TEST(Prefix67Test32, SixteenBit_addressing_has_no_SIB_byte)
{
    /* mod=00 r/m=100 means [SI] in 16-bit addressing, but "SIB follows" in
     * 32-bit addressing - the sharpest difference between the two maps. */
    EXPECT_EQ(x86({ 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 7u);
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x04 }), 3u); // [SI]
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x07 }), 3u); // [BX]
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x01 }), 3u); // [BX+SI]
}

TEST(Prefix67Test32, Mod10_takes_disp16_instead_of_disp32)
{
    EXPECT_EQ(x86({ 0x8D, 0x80, 1, 2, 3, 4 }), 6u);       // LEA eax, [eax+disp32]
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x80, 1, 2 }), 5u);       // LEA ax, [BX+disp16]
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x84, 1, 2 }), 5u);       // [SI]+disp16, still no SIB
}

TEST(Prefix67Test32, Mod01_keeps_disp8)
{
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x44, 0x08 }), 4u); // [SI]+disp8
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x40, 0x08 }), 4u); // [BX+SI]+disp8
    EXPECT_EQ(x86({ 0x8D, 0x44, 0x08, 0x10 }), 4u); // 32-bit form: SIB + disp8
}

TEST(Prefix67Test32, Reg_modrm_ignores_address_size)
{
    /* mod=11 has no displacement in either address size.  MOV rather than LEA:
     * the latter cannot name a register at all. */
    EXPECT_EQ(x86({ 0x8B, 0xC0 }), 2u);
    EXPECT_EQ(x86({ 0x67, 0x8B, 0xC0 }), 3u);
}

TEST(Prefix67Test32, Group_and_string_forms_follow_address_size)
{
    EXPECT_EQ(x86({ 0xFF, 0x16, 1, 2, 3, 4 }), 2u);          // CALL far [esi]: the far pointer lives in memory
    EXPECT_EQ(x86({ 0x67, 0xFF, 0x16, 1, 2 }), 5u);          // CALL far [disp16]
    EXPECT_EQ(x86({ 0x67, 0xFE, 0x00 }), 3u);                // INC byte [BX]
    EXPECT_EQ(x86({ 0x67, 0xF7, 0x00, 1, 2, 3, 4 }), 7u);    // TEST dword [BX], imm32
}

TEST(Prefix67Test32, Operand_size_and_address_size_together)
{
    EXPECT_EQ(x86({ 0x66, 0x67, 0x8D, 0x06, 1, 2 }), 6u); // LEA si, [disp16]
    EXPECT_EQ(x86({ 0x66, 0x67, 0xA1, 1, 2 }), 5u);       // MOV ax, moffs16
}

TEST(Prefix67Test32, Duplicate_67_counts_toward_length)
{
    EXPECT_EQ(x86({ 0x67, 0x67, 0x90 }), 3u);
}

TEST(Prefix67Test32, Truncated_disp16_reports_insufficient)
{
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x06, 1 }), ERR_INSUFFICIENT); // disp16 is cut short
    EXPECT_EQ(x86({ 0x67, 0xA1, 1 }), ERR_INSUFFICIENT);       // moffs16 is cut short
}

TEST(Prefix67Test32, Sixty_six_increments_are_opcodes_here)
{
    /* 0x67 does not turn 0x66-prefixed INC/DEC into anything else. */
    EXPECT_EQ(x86({ 0x67, 0x40 }), 2u); // INC eax after an address-size prefix
}
