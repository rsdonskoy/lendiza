// test_prefixes.cpp
// Coverage for the prefix model itself: which bytes are prefixes, how they stack,
// which repetitions are illegal, and what happens to VEX/EVEX.
//
// The rules mirror xendiza's ProcessPrefix: legacy prefixes belong to the
// instruction they precede and are counted into its length; 0x66, 0x67, 0xF0,
// F2/F3 and REX may each appear at most once; segment overrides may stack (the
// last one wins); VEX (C4/C5) and EVEX (62) are rejected in long mode, where the
// decoder does not implement them, and are ordinary opcodes in IA-32.
//
// The 0x66/0x67 *width* consequences live in test_prefix_operand_size.cpp and
// test_prefix_address_size.cpp; this file is about grouping and legality.
//
// Suites: PrefixTest (x64), PrefixTest32 (IA-32).

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

// -----------------------------------------------------------------------------
// REX (long mode only).
// -----------------------------------------------------------------------------

TEST(PrefixTest, REX_all_sixteen_values_count_as_one_prefix_byte)
{
    for (uint8_t rex = 0x40; rex <= 0x4F; ++rex) {
        const bool widens = (rex & 0x08) != 0;                       /* REX.W */
        EXPECT_EQ(x64({ rex, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), widens ? 10u : 6u);
    }
}

TEST(PrefixTest, REX_W_selects_imm64_for_B8_only)
{
    EXPECT_EQ(x64({ 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u); // MOV rax, imm64
    EXPECT_EQ(x64({ 0x48, 0x81, 0xC0, 1, 2, 3, 4 }), 7u);        // ADD rax, imm32 (sign-extended)
    EXPECT_EQ(x64({ 0x48, 0xF7, 0xC0, 1, 2, 3, 4 }), 7u);        // TEST rax, imm32
    EXPECT_EQ(x64({ 0x48, 0xE8, 1, 2, 3, 4 }), 6u);              // CALL rel32
}

TEST(PrefixTest, SIB_base5_keeps_its_disp32_under_REX_B)
{
    EXPECT_EQ(x64({ 0x40, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 8u); // REX + opcode + ModRM + SIB + disp32
    EXPECT_EQ(x64({ 0x41, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 8u); /* REX.B: still disp32 */
    EXPECT_EQ(x64({ 0x4D, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 8u); /* REX.WRXB: same */

    /* Control, measured on the same CPU: at mod=01 there is no "no base" case,
     * and REX.B really does select r13 rather than rbp.  So only the
     * displacement rule above holds; REX.B is not being ignored wholesale. */
    EXPECT_EQ(x64({ 0x41, 0x8D, 0x44, 0x25, 0x7F }), 5u);
}

TEST(PrefixTest, Repeated_REX_takes_the_last_one)
{
    /* Hardware folds every REX byte into the same instruction; the one nearest
     * the opcode is the effective one. */
    EXPECT_EQ(x64({ 0x48, 0x48, 0x90 }), 3u);
    EXPECT_EQ(x64({ 0x40, 0x4F, 0x90 }), 3u);
    EXPECT_EQ(x64({ 0x4C, 0x41, 0xB8, 1, 2, 3, 4 }), 7u); /* REX.B, no W -> imm32 */
}

TEST(PrefixTest, Legacy_prefix_after_REX_voids_it)
{
    /* A REX only applies when it is the last prefix before the opcode.  These
     * widths are what the CPU returns when the encodings are executed: the
     * leading 0x48 is dropped, so the 0x66 form stays 16-bit and the immediate
     * is not widened to 64 bits. */
    EXPECT_EQ(x64({ 0x48, 0x66, 0xB8, 0xEF, 0xBE }), 5u);
    EXPECT_EQ(x64({ 0x48, 0x66, 0x66, 0xB8, 0xEF, 0xBE }), 6u);
    EXPECT_EQ(x64({ 0x48, 0x67, 0xB8, 1, 2, 3, 4 }), 7u);
    EXPECT_EQ(x64({ 0x48, 0x64, 0xB8, 1, 2, 3, 4 }), 7u);
    EXPECT_EQ(x64({ 0x66, 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 11u); /* REX last: wins */

    /* Not just the W bit: with the REX.B void, SIB.base=101 keeps its disp32.
     * A surviving REX.B would address [r13] and shrink this to 5. */
    EXPECT_EQ(x64({ 0x41, 0x66, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 9u);
}

TEST(PrefixTest, REX_after_the_opcode_is_not_a_prefix)
{
    EXPECT_EQ(x64({ 0x90, 0x48, 0xB8 }), 1u); /* NOP; the REX belongs to what follows */
    EXPECT_EQ(x64({ 0xC3, 0x48 }), 1u);
}

// -----------------------------------------------------------------------------
// LOCK / REP / segment overrides.
// -----------------------------------------------------------------------------

TEST(PrefixTest, Lock_and_rep_are_counted)
{
    EXPECT_EQ(x64({ 0xF0, 0x90 }), 2u);   // LOCK XCHG? (legacy prefix + NOP)
    EXPECT_EQ(x64({ 0xF2, 0xA5 }), 2u);   // REPNE STOSD family form
    EXPECT_EQ(x64({ 0xF3, 0xA5 }), 2u);   // REP MOVSD
    EXPECT_EQ(x64({ 0xF3, 0x0F, 0x1E, 0xFA }), 4u); // ENDBR64
}

TEST(PrefixTest, Duplicate_lock_or_rep_counts_toward_length)
{
    /* The CPU holds one slot per prefix type, so repeats overwrite rather than
     * fault; every repeat byte still belongs to the instruction. */
    EXPECT_EQ(x64({ 0xF0, 0xF0, 0x90 }), 3u);
    EXPECT_EQ(x64({ 0xF3, 0xF2, 0x90 }), 3u);
    EXPECT_EQ(x64({ 0xF2, 0xF3, 0x90 }), 3u);
    EXPECT_EQ(x64({ 0xF3, 0xF3, 0x90 }), 3u);
}

TEST(PrefixTest, Segment_overrides_stack_and_all_count)
{
    EXPECT_EQ(x64({ 0x64, 0x90 }), 2u);
    EXPECT_EQ(x64({ 0x64, 0x65, 0x90 }), 3u); /* last override wins, both are bytes */
    EXPECT_EQ(x64({ 0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0x90 }), 7u);
    EXPECT_EQ(x64({ 0x64, 0x48, 0x8B, 0x00 }), 4u); // MOV rax, QWORD PTR FS:[rax]
}

TEST(PrefixTest, Prefix_ordering_matters_only_for_REX)
{
    /* The same seven prefixes in two orders, which no longer agree: order only
     * decides whether the REX still applies. */

    /* REX last: it widens the immediate to 64 bits, so 7 + 1 + 8 = 16 bytes -
     * longer than any legal instruction, rejected whichever order. */
    EXPECT_EQ(x64({ 0x64, 0x66, 0x67, 0xF0, 0xF2, 0x26, 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }),
              ERR_UNDEFINED);

    /* REX first: the 0x66 that follows voids it, so the immediate is 16 bits and
     * the whole thing fits in 7 + 1 + 2 = 10 bytes. Measured on hardware. */
    EXPECT_EQ(x64({ 0x48, 0x26, 0xF2, 0xF0, 0x67, 0x66, 0x65, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u);

    /* Six of them plus an 8-byte immediate is exactly 15 bytes: still legal. */
    EXPECT_EQ(x64({ 0x66, 0x67, 0xF0, 0xF2, 0x26, 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 15u);
}

TEST(PrefixTest, ZeroF_is_an_escape_not_a_consumed_prefix)
{
    EXPECT_EQ(x64({ 0x0F, 0xA2 }), 2u);      // CPUID
    EXPECT_EQ(x64({ 0x66, 0x0F, 0xA2 }), 3u); // 66 is the prefix; 0F starts the opcode
    EXPECT_EQ(x64({ 0x0F }), ERR_INSUFFICIENT);
}

// -----------------------------------------------------------------------------
// VEX / EVEX.
// -----------------------------------------------------------------------------

TEST(PrefixTest, Vex_and_evex_are_reported_as_undefined)
{
    EXPECT_EQ(x64({ 0xC5, 0xF8, 0x29, 0xC0 }), ERR_UNDEFINED);       // VEX.128
    EXPECT_EQ(x64({ 0xC4, 0xE1, 0xF8, 0x29, 0xC0 }), ERR_UNDEFINED); // VEX.3
    EXPECT_EQ(x64({ 0x62, 0xF1, 0xFD, 0x48, 0x29, 0xC0 }), ERR_UNDEFINED); // EVEX
    EXPECT_EQ(x64({ 0xC5 }), ERR_UNDEFINED);
    /* An illegal VEX byte after a real opcode is just the next instruction. */
    EXPECT_EQ(x64({ 0x90, 0xC5, 0xF8, 0x29 }), 1u);
}

// -----------------------------------------------------------------------------
// IA-32: same grouping rules, different prefix set.
// -----------------------------------------------------------------------------

TEST(PrefixTest32, Inc_dec_bytes_are_never_prefixes_here)
{
    for (uint8_t b = 0x40; b <= 0x4F; ++b) {
        EXPECT_EQ(x86({ b, 0x90 }), 1u); // INC/DEC eax..edi, then NOP separately
    }
    EXPECT_EQ(x86({ 0x48, 0x48, 0x90 }), 1u); // no "duplicate REX" rule to trigger
}

TEST(PrefixTest32, Bound_and_les_lds_are_opcodes_not_vex)
{
    EXPECT_EQ(x86({ 0x62, 0xC1 }), 2u);            // BOUND eax, [ecx]
    EXPECT_EQ(x86({ 0xC4, 0xC1 }), 2u);      // LES with mod=11: opcode + ModRM only
    EXPECT_EQ(x86({ 0xC5, 0x45, 0x10 }), 3u);      // LDS eax, [ebp+0x10]
    EXPECT_EQ(x86({ 0x62 }), ERR_INSUFFICIENT);    // needs a ModRM byte
    EXPECT_EQ(x86({ 0xC4 }), ERR_INSUFFICIENT);
}

TEST(PrefixTest32, Legacy_prefix_grouping_matches_long_mode)
{
    EXPECT_EQ(x86({ 0x64, 0x67, 0x90 }), 3u);
    EXPECT_EQ(x86({ 0xF0, 0xF0, 0x90 }), 3u);
    EXPECT_EQ(x86({ 0x66, 0x66, 0x90 }), 3u);
    EXPECT_EQ(x86({ 0x67, 0x67, 0x90 }), 3u);
    EXPECT_EQ(x86({ 0xF3, 0xA5 }), 2u);   // REP MOVSD
    EXPECT_EQ(x86({ 0xF3 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x86({ 0x0F }), ERR_INSUFFICIENT);
}

TEST(PrefixTest32, Prefixes_stack_up_to_the_instruction_limit)
{
    /* Five prefixes + opcode + a 0x66-shrunk immediate = 8 bytes. */
    EXPECT_EQ(x86({ 0x66, 0x67, 0xF0, 0xF2, 0x26, 0xB8, 1, 2 }), 8u);
}
