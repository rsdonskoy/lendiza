// test_prefix_operand_size.cpp
// Coverage for the 0x66 operand-size override in both decode modes.
//
// 0x66 belongs to the instruction it precedes, so two things must hold: the
// prefix is counted into the returned length, and the trailing immediate (or
// relative offset) is measured at 16 bits instead of 32.  Both are asserted per
// instruction family below, together with the forms 0x66 must NOT resize
// (imm8, displacements, moffs, SIB) and the REX.W precedence for MOV r,imm.
//
// Expected lengths are read off the Intel encoding tables, not off the
// implementation: each case names the instruction it stands for.
//
// Suites: Prefix66Test (x64), Prefix66Test32 (IA-32).

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

// -----------------------------------------------------------------------------
// Iz forms: imm32 shrinks to imm16.
// -----------------------------------------------------------------------------

TEST(Prefix66Test, ADD_0x05_Iz_imm16)
{
    EXPECT_EQ(x64({ 0x05, 0x78, 0x56, 0x34, 0x12 }), 5u); // ADD eax, 0x12345678
    EXPECT_EQ(x64({ 0x66, 0x05, 0x78, 0x56 }), 4u);       // ADD ax, 0x5678
}

TEST(Prefix66Test, ALU_0x0D_0x15_0x1D_0x25_0x2D_0x35_0x3D_Iz_imm16)
{
    /* OR/ADC/SBB/AND/SUB/XOR/CMP with the accumulator take an Iz. */
    static const uint8_t iz_ops[] = { 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D };
    for (const uint8_t opc : iz_ops) {
        EXPECT_EQ(x64({ opc, 0x78, 0x56, 0x34, 0x12 }), 5u);
        EXPECT_EQ(x64({ 0x66, opc, 0x78, 0x56 }), 4u);
    }
}

TEST(Prefix66Test, Group1_0x81_Iz_imm16)
{
    EXPECT_EQ(x64({ 0x81, 0xC0, 0x78, 0x56, 0x34, 0x12 }), 6u); // ADD eax, imm32
    EXPECT_EQ(x64({ 0x66, 0x81, 0xC0, 0x78, 0x56 }), 5u);       // ADD ax, imm16 (prefix counted)
}

TEST(Prefix66Test, IMUL_0x69_Iz_imm16)
{
    EXPECT_EQ(x64({ 0x69, 0xC0, 0x78, 0x56, 0x34, 0x12 }), 6u); // IMUL eax, eax, imm32
    EXPECT_EQ(x64({ 0x66, 0x69, 0xC0, 0x78, 0x56 }), 5u);       // IMUL ax, ax, imm16
}

TEST(Prefix66Test, Group_0xF7_Iz_imm16)
{
    EXPECT_EQ(x64({ 0xF7, 0xC0, 0x78, 0x56, 0x34, 0x12 }), 6u); // TEST eax, imm32
    EXPECT_EQ(x64({ 0x66, 0xF7, 0xC0, 0x78, 0x56 }), 5u);       // TEST ax, imm16
}

TEST(Prefix66Test, Group_0xC7_Iz_imm16)
{
    EXPECT_EQ(x64({ 0xC7, 0xC0, 0x78, 0x56, 0x34, 0x12 }), 6u); // MOV eax, imm32
    EXPECT_EQ(x64({ 0x66, 0xC7, 0xC0, 0x78, 0x56 }), 5u);       // MOV ax, imm16
}

TEST(Prefix66Test, PUSH_0x68_Iz_imm16)
{
    EXPECT_EQ(x64({ 0x68, 0x78, 0x56, 0x34, 0x12 }), 5u); // PUSH imm32
    EXPECT_EQ(x64({ 0x66, 0x68, 0x78, 0x56 }), 4u);       // PUSH imm16
}

TEST(Prefix66Test, TEST_0xA9_Iz_imm16)
{
    EXPECT_EQ(x64({ 0xA9, 0x78, 0x56, 0x34, 0x12 }), 5u); // TEST eax, imm32
    EXPECT_EQ(x64({ 0x66, 0xA9, 0x78, 0x56 }), 4u);       // TEST ax, imm16
}

// -----------------------------------------------------------------------------
// MOV r,imm (B8-BF): the one form REX.W widens to imm64.  REX.W wins over 0x66.
// -----------------------------------------------------------------------------

TEST(Prefix66Test, MOV_0xB8_plain_imm32)
{
    EXPECT_EQ(x64({ 0xB8, 0x78, 0x56, 0x34, 0x12 }), 5u); // MOV eax, imm32
}

TEST(Prefix66Test, MOV_0xB8_Prefix66_imm16)
{
    EXPECT_EQ(x64({ 0x66, 0xB8, 0x78, 0x56 }), 4u); // MOV ax, imm16
}

TEST(Prefix66Test, MOV_0xB8_REX_W_imm64)
{
    EXPECT_EQ(x64({ 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u); // MOV rax, imm64
}

TEST(Prefix66Test, MOV_0xB8_REX_W_Prefix66_rex_w_wins)
{
    EXPECT_EQ(x64({ 0x66, 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 11u); // 2 prefix bytes + opcode + imm64
    EXPECT_EQ(x64({ 0x48, 0x66, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 11u); // order irrelevant
}

TEST(Prefix66Test, MOV_0xBF_REX_W_imm64)
{
    EXPECT_EQ(x64({ 0x4F, 0xBF, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u); // MOV r15, imm64
    EXPECT_EQ(x64({ 0x66, 0xBF, 0x78, 0x56 }), 4u);              // MOV di, imm16
}

// -----------------------------------------------------------------------------
// Relative branches: rel32 shrinks to rel16.
// -----------------------------------------------------------------------------

TEST(Prefix66Test, CALL_0xE8_relz)
{
    EXPECT_EQ(x64({ 0xE8, 0xFB, 0xFF, 0xFF, 0xFF }), 5u);
    EXPECT_EQ(x64({ 0x66, 0xE8, 0xFB, 0xFF }), 4u); // prefix + opcode + rel16
}

TEST(Prefix66Test, JMP_0xE9_relz)
{
    EXPECT_EQ(x64({ 0xE9, 0xFB, 0xFF, 0xFF, 0xFF }), 5u);
    EXPECT_EQ(x64({ 0x66, 0xE9, 0xFB, 0xFF }), 4u);
}

TEST(Prefix66Test, Jcc_0x0F80_relz)
{
    EXPECT_EQ(x64({ 0x0F, 0x80, 0x78, 0x56, 0x34, 0x12 }), 6u); // JO rel32
    EXPECT_EQ(x64({ 0x66, 0x0F, 0x80, 0x78, 0x56 }), 5u);       // prefix + 2 opcode bytes + rel16
}

TEST(Prefix66Test, Jcc_0x0F8F_relz_last_condition)
{
    EXPECT_EQ(x64({ 0x0F, 0x8F, 0x78, 0x56, 0x34, 0x12 }), 6u); // JG rel32
    EXPECT_EQ(x64({ 0x66, 0x0F, 0x8F, 0x78, 0x56 }), 5u);       // JG rel16
}

TEST(Prefix66Test, Jcc_0x70_rel8_unaffected)
{
    EXPECT_EQ(x64({ 0x70, 0x12 }), 2u);
    EXPECT_EQ(x64({ 0x66, 0x70, 0x12 }), 3u); // counted as a prefix only
}

// -----------------------------------------------------------------------------
// What 0x66 must not resize: imm8, displacements, moffs, SIB.
// -----------------------------------------------------------------------------

TEST(Prefix66Test, Group1_0x83_Ib_unaffected)
{
    EXPECT_EQ(x64({ 0x83, 0xC0, 0x05 }), 3u);
    EXPECT_EQ(x64({ 0x66, 0x83, 0xC0, 0x05 }), 4u); // only the prefix is added
}

TEST(Prefix66Test, Memory_disp32_unaffected_by_66)
{
    EXPECT_EQ(x64({ 0x03, 0x05, 0x78, 0x56, 0x34, 0x12 }), 6u); // ADD eax, [disp32]
    EXPECT_EQ(x64({ 0x66, 0x03, 0x05, 0x78, 0x56, 0x34, 0x12 }), 7u);
}

TEST(Prefix66Test, Memory_SIB_disp32_unaffected_by_66)
{
    EXPECT_EQ(x64({ 0x8B, 0x04, 0x25, 0x78, 0x56, 0x34, 0x12 }), 7u); // MOV eax, [disp32*1]
    EXPECT_EQ(x64({ 0x66, 0x8B, 0x04, 0x25, 0x78, 0x56, 0x34, 0x12 }), 8u);
}

TEST(Prefix66Test, Moffs_0xA1_governed_by_address_size_not_66)
{
    EXPECT_EQ(x64({ 0xA1, 1, 2, 3, 4, 5, 6, 7, 8 }), 9u);           // MOV eax, moffs64
    EXPECT_EQ(x64({ 0x66, 0xA1, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u);    // prefix + moffs64
}

TEST(Prefix66Test, NoOperand_forms_only_grow_by_the_prefix)
{
    EXPECT_EQ(x64({ 0x90 }), 1u);
    EXPECT_EQ(x64({ 0x66, 0x90 }), 2u);   // XCHG ax, ax
    EXPECT_EQ(x64({ 0x66, 0xC3 }), 2u);   // RET
    EXPECT_EQ(x64({ 0x66, 0xF4 }), 2u);   // HLT
    EXPECT_EQ(x64({ 0x66, 0xFF, 0xD0 }), 3u); // CALL ax (FF /2, mod=11)
}

TEST(Prefix66Test, Duplicate_66_is_undefined)
{
    EXPECT_EQ(x64({ 0x66, 0x66, 0x90 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x66, 0x66, 0xB8, 0x78, 0x56 }), ERR_UNDEFINED);
}

TEST(Prefix66Test, Operand_size_plus_other_prefixes)
{
    EXPECT_EQ(x64({ 0x66, 0xF0, 0x81, 0xC0, 0x78, 0x56 }), 6u);      // LOCK ADD ax, imm16
    EXPECT_EQ(x64({ 0x64, 0x66, 0x81, 0xC0, 0x78, 0x56 }), 6u);      // FS ADD ax, imm16
    EXPECT_EQ(x64({ 0x66, 0x67, 0x8B, 0x04, 0x25, 1, 2, 3, 4 }), 9u); // 66+67, SIB disp32
}

// -----------------------------------------------------------------------------
// IA-32: same rules, plus the forms that only exist in 32-bit mode.
// -----------------------------------------------------------------------------

TEST(Prefix66Test32, ALU_Iz_imm16)
{
    static const uint8_t iz_ops[] = { 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D };
    for (const uint8_t opc : iz_ops) {
        EXPECT_EQ(x86({ opc, 0x78, 0x56, 0x34, 0x12 }), 5u);
        EXPECT_EQ(x86({ 0x66, opc, 0x78, 0x56 }), 4u);
    }
}

TEST(Prefix66Test32, MOV_0xB8_Iz_imm16)
{
    EXPECT_EQ(x86({ 0xB8, 0x78, 0x56, 0x34, 0x12 }), 5u); // MOV eax, imm32
    EXPECT_EQ(x86({ 0x66, 0xB8, 0x78, 0x56 }), 4u);       // MOV ax, imm16
}

TEST(Prefix66Test32, PUSH_0x68_Iz_imm16)
{
    EXPECT_EQ(x86({ 0x68, 0x78, 0x56, 0x34, 0x12 }), 5u);
    EXPECT_EQ(x86({ 0x66, 0x68, 0x78, 0x56 }), 4u);
}

TEST(Prefix66Test32, CALL_far_0x9A_ptr16_32_to_ptr16_16)
{
    EXPECT_EQ(x86({ 0x9A, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00 }), 7u); // CALL ptr16:32
    EXPECT_EQ(x86({ 0x66, 0x9A, 0x78, 0x56, 0x34, 0x12 }), 6u);       // CALL ptr16:16
}

TEST(Prefix66Test32, JMP_far_0xEA_ptr16_32_to_ptr16_16)
{
    EXPECT_EQ(x86({ 0xEA, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00 }), 7u); // JMP ptr16:32
    EXPECT_EQ(x86({ 0x66, 0xEA, 0x78, 0x56, 0x34, 0x12 }), 6u);       // JMP ptr16:16
}

TEST(Prefix66Test32, INC_0x40_r16_still_one_byte)
{
    EXPECT_EQ(x86({ 0x40 }), 1u);          // INC eax
    EXPECT_EQ(x86({ 0x66, 0x40 }), 2u);    // INC ax - width changed, length did not
    EXPECT_EQ(x86({ 0x66, 0x4F }), 2u);    // DEC DI
}

TEST(Prefix66Test32, ENTER_0xC8_Iw_Ib_unaffected)
{
    EXPECT_EQ(x86({ 0xC8, 0x00, 0x00, 0x05 }), 4u);
    EXPECT_EQ(x86({ 0x66, 0xC8, 0x00, 0x00, 0x05 }), 5u);
}

TEST(Prefix66Test32, Jcc_0x0F80_relz)
{
    EXPECT_EQ(x86({ 0x0F, 0x80, 0x78, 0x56, 0x34, 0x12 }), 6u);
    EXPECT_EQ(x86({ 0x66, 0x0F, 0x80, 0x78, 0x56 }), 5u);
}

TEST(Prefix66Test32, Moffs_0xA1_governed_by_address_size)
{
    EXPECT_EQ(x86({ 0xA1, 0x78, 0x56, 0x34, 0x12 }), 5u);        // MOV eax, moffs32
    EXPECT_EQ(x86({ 0x66, 0xA1, 0x78, 0x56, 0x34, 0x12 }), 6u);  // prefix only
}

TEST(Prefix66Test32, Group_0xF7_Iz_imm16)
{
    EXPECT_EQ(x86({ 0xF7, 0xC0, 0x78, 0x56, 0x34, 0x12 }), 6u);
    EXPECT_EQ(x86({ 0x66, 0xF7, 0xC0, 0x78, 0x56 }), 5u);
}

TEST(Prefix66Test32, Duplicate_66_is_undefined)
{
    EXPECT_EQ(x86({ 0x66, 0x66, 0x90 }), ERR_UNDEFINED);
}

TEST(Prefix66Test32, Rex_bytes_are_opcodes_in_ia32)
{
    /* 0x40-0x4F are INC/DEC here, never prefixes - the biggest 32/64 split. */
    EXPECT_EQ(x86({ 0x48, 0xB8, 0x78, 0x56, 0x34, 0x12 }), 1u); // DEC eax, then a separate MOV
    EXPECT_EQ(x86({ 0x48 }), 1u);
    EXPECT_EQ(x86({ 0x4B }), 1u);
}
