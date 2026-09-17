// test_boundary.cpp
// Kernel-safety coverage: the decoder must never report a length it could not
// have read, and must never read past the buffer the caller guaranteed.
//
// In a driver an over-read is a bugcheck, not a wrong answer, so the invariant
// below is asserted over the whole opcode space rather than spot-checked:
//
//     for every input and every buffer length n,  result <= n  or  result is
//     an error code.
//
// That single property is what lets kernel callers hand the decoder a truncated
// window at the end of a page.  The remaining cases pin the specific boundaries
// where the old implementation used to read a byte it did not have (ModRM that
// implies a SIB, disp32, imm64, the 0F 38 / 0F 3A escapes) and the 15-byte
// instruction limit.
//
// Suites: BoundaryTest (x64), BoundaryTest32 (IA-32).

#include <cstdint>
#include <vector>

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

namespace {

/* Filler tail bytes that are themselves plausible ModRM/SIB/disp values, so the
 * walk exercises the widest variety of encodings. */
const std::vector<uint8_t>& tails()
{
    static const std::vector<uint8_t> t = { 0x00, 0x05, 0x0D, 0x24, 0x25, 0x35, 0x44, 0x8D, 0xA5,
                                            0xC3, 0xC5, 0xE8, 0xFF, 0x80, 0x0F, 0x3A };
    return t;
}

/* Feeds one opcode plus a tail, truncating the buffer at every length, and
 * counts any answer that reaches beyond the bytes it was given. */
template <typename Decode>
size_t count_overruns(Decode decode, bool with_second_byte)
{
    size_t violations = 0;
    const std::vector<uint8_t>& t = tails();

    for (uint32_t opc = 0; opc <= 0xFF; ++opc) {
        for (size_t ti = 0; ti < t.size(); ++ti) {
            for (size_t tj = 0; tj < t.size(); ++tj) {
                std::vector<uint8_t> full;
                full.push_back(static_cast<uint8_t>(opc));
                full.push_back(t[ti]);
                full.push_back(t[tj]);
                if (with_second_byte) {
                    full.push_back(0x41);
                    full.push_back(0x0D);
                    full.push_back(0x66);
                    full.push_back(0x8D);
                }
                while (full.size() < 15) {
                    full.push_back(0x05);
                }

                for (size_t n = 1; n <= full.size(); ++n) {
                    const size_t len = decode(full.data(), n);
                    if (len < 0xE0 && len > n) {
                        ++violations;
                    }
                    /* A prefix-only window must never look like an instruction. */
                    if (len == 0 && n > 0) {
                        ++violations;
                    }
                }
            }
        }
    }
    return violations;
}

} // namespace

// -----------------------------------------------------------------------------
// The invariant, over the whole opcode space.
// -----------------------------------------------------------------------------

TEST(BoundaryTest, NeverReturnsALengthBeyondTheGivenBuffer)
{
    const size_t violations =
        count_overruns([](const uint8_t* p, size_t n) { return lendiza::detail::amd64traits::ldiza(p, n); },
                       false);
    EXPECT_EQ(violations, 0u);
}

TEST(BoundaryTest, SameInvariant_ForTwoAndThreeByteEscapes)
{
    const size_t violations =
        count_overruns([](const uint8_t* p, size_t n) { return lendiza::detail::amd64traits::ldiza(p, n); },
                       true);
    EXPECT_EQ(violations, 0u);
}

TEST(BoundaryTest32, NeverReturnsALengthBeyondTheGivenBuffer)
{
    const size_t violations =
        count_overruns([](const uint8_t* p, size_t n) { return lendiza::detail::x86traits::ldiza(p, n); },
                       false);
    EXPECT_EQ(violations, 0u);
}

TEST(BoundaryTest32, SameInvariant_ForTwoByteEscapes)
{
    const size_t violations =
        count_overruns([](const uint8_t* p, size_t n) { return lendiza::detail::x86traits::ldiza(p, n); },
                       true);
    EXPECT_EQ(violations, 0u);
}

// -----------------------------------------------------------------------------
// Named boundaries, one assertion each, so a regression points at a form.
// -----------------------------------------------------------------------------

TEST(BoundaryTest, Modrm_implying_SIB_needs_the_SIB_byte)
{
    EXPECT_EQ(x64({ 0x8D, 0x04 }), ERR_INSUFFICIENT);          // [SIB follows]
    EXPECT_EQ(x64({ 0x8D, 0x04, 0x25 }), ERR_INSUFFICIENT);    // SIB says base=101 -> disp32
    EXPECT_EQ(x64({ 0x8D, 0x04, 0x25, 1 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x8D, 0x04, 0x25, 1, 2, 3 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 7u);      // complete
}

TEST(BoundaryTest, Rip_relative_needs_all_four_displacement_bytes)
{
    EXPECT_EQ(x64({ 0x8B, 0x05 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x8B, 0x05, 1, 2, 3 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x8B, 0x05, 1, 2, 3, 4 }), 6u);
    EXPECT_EQ(x64({ 0x48, 0x8B, 0x05, 1, 2, 3, 4 }), 7u); // REX counted, still one instruction
}

TEST(BoundaryTest, REXB_sib_base5_still_needs_its_disp32)
{
    EXPECT_EQ(x64({ 0x49, 0x8D, 0x04, 0x25 }), ERR_INSUFFICIENT); /* disp32 cut short */
    EXPECT_EQ(x64({ 0x49, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 8u);
    EXPECT_EQ(x64({ 0x49, 0x8D, 0x04 }), ERR_INSUFFICIENT); // SIB byte itself is missing
}

TEST(BoundaryTest, AddressSizePrefix_restoresTheRequirementForDisp32)
{
    /* Under 0x67 the base=101 exception disappears, so the same encoding is now
     * an 9-byte instruction and a 5-byte window is not enough. */
    EXPECT_EQ(x64({ 0x67, 0x49, 0x8D, 0x04, 0x25 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x67, 0x49, 0x8D, 0x04, 0x25, 1, 2, 3 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x67, 0x49, 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 9u);
}

TEST(BoundaryTest, Immediates_need_every_byte)
{
    EXPECT_EQ(x64({ 0xB8, 1, 2, 3 }), ERR_INSUFFICIENT);           // imm32 cut short
    EXPECT_EQ(x64({ 0xB8, 1, 2, 3, 4 }), 5u);
    EXPECT_EQ(x64({ 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7 }), ERR_INSUFFICIENT); // imm64 cut short
    EXPECT_EQ(x64({ 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u);
    EXPECT_EQ(x64({ 0x66, 0xB8, 1 }), ERR_INSUFFICIENT);           // imm16 cut short
    EXPECT_EQ(x64({ 0x66, 0xB8, 1, 2 }), 4u);
    EXPECT_EQ(x64({ 0xC7, 0x05, 1, 2, 3, 4 }), ERR_INSUFFICIENT);  // disp32 + imm32
    EXPECT_EQ(x64({ 0xC7, 0x05, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u);
}

TEST(BoundaryTest, Group3_immediates_are_counted_before_being_accepted)
{
    EXPECT_EQ(x64({ 0xF7, 0x05, 1, 2, 3 }), ERR_INSUFFICIENT); // [rip+d32] + imm32 = 10
    EXPECT_EQ(x64({ 0xF7, 0x05, 1, 2, 3, 4, 5, 6, 7, 8 }), 10u);
    EXPECT_EQ(x64({ 0xF6, 0x05, 1, 2 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0xF6, 0x05, 1, 2, 3, 4, 5 }), 7u); // disp32 + imm8
}

TEST(BoundaryTest, ThreeByteEscapes_do_not_read_past_their_table_byte)
{
    EXPECT_EQ(x64({ 0x0F, 0x38 }), ERR_INSUFFICIENT);
    /* Every defined 0F 38 / 0F 3A entry carries a ModRM byte, so three bytes are
     * never a complete encoding. */
    EXPECT_EQ(x64({ 0x0F, 0x38, 0xF0 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x0F, 0x38, 0xF0, 0xC1 }), 4u);                   // MOVBE ax, cx
    EXPECT_EQ(x64({ 0x0F, 0x38, 0xF0, 0x04 }), ERR_INSUFFICIENT);     // ModRM says SIB follows
    EXPECT_EQ(x64({ 0x0F, 0x38, 0xF0, 0x04, 0x25, 1, 2, 3, 4 }), 9u); // SIB + disp32
    EXPECT_EQ(x64({ 0x0F, 0x3A, 0x0F, 0xC0, 0x08 }), 5u);             // PALIGNR mm, mm/m64, imm8
    EXPECT_EQ(x64({ 0x0F, 0x3A, 0x0F, 0xC0 }), ERR_INSUFFICIENT);     // imm8 missing
    EXPECT_EQ(x64({ 0x0F, 0x3A, 0x00 }), ERR_UNDEFINED);              // undefined in the table
    EXPECT_EQ(x64({ 0x0F, 0x38, 0x00 }), ERR_INSUFFICIENT);          // PSHUFB wants a ModRM
}

TEST(BoundaryTest, PrefixBytes_alone_are_not_an_instruction)
{
    static const uint8_t prefix_bytes[] = { 0x66, 0x67, 0xF0, 0xF2, 0xF3, 0x40, 0x48 };
    for (const uint8_t p : prefix_bytes) {
        EXPECT_EQ(x64({ p }), ERR_INSUFFICIENT);
    }

    /* Repeating a legacy prefix overwrites the same slot in the CPU, and a
     * repeated REX leaves the last one in effect; either way both bytes belong
     * to the instruction. */
    for (const uint8_t p : prefix_bytes) {
        EXPECT_EQ(x64({ p, p, 0x90 }), 3u);
    }

    /* Segment overrides may stack; the last one wins, and all of them count. */
    static const uint8_t segs[] = { 0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65 };
    for (const uint8_t p : segs) {
        EXPECT_EQ(x64({ p, p, 0x90 }), 3u);
    }
    EXPECT_EQ(x64({ 0x66, 0x67, 0xF0 }), ERR_INSUFFICIENT);
}

TEST(BoundaryTest, FifteenByte_limit_is_enforced)
{
    /* 66 67 F0 F2 26 64 48 B8 + imm64 = 8 prefix bytes + opcode + imm64 = 17. */
    EXPECT_EQ(x64({ 0x66, 0x67, 0xF0, 0xF2, 0x26, 0x64, 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7 }),
              ERR_UNDEFINED);
    /* Exactly 15 bytes is still a legal instruction. */
    EXPECT_EQ(x64({ 0x66, 0x67, 0xF0, 0x26, 0x64, 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }), 15u);
}

TEST(BoundaryTest32, Modrm_implying_SIB_and_disp32)
{
    EXPECT_EQ(x86({ 0x8D, 0x04 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x86({ 0x8D, 0x04, 0x25, 1, 2, 3 }), ERR_INSUFFICIENT);
    EXPECT_EQ(x86({ 0x8D, 0x04, 0x25, 1, 2, 3, 4 }), 7u);
    EXPECT_EQ(x86({ 0x8D, 0x05, 1, 2, 3 }), ERR_INSUFFICIENT); // [ebx+disp32]
    EXPECT_EQ(x86({ 0x8D, 0x05, 1, 2, 3, 4 }), 6u);
}

TEST(Prefix67Test32, SixteenBit_disp16_boundaries)
{
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x80, 1 }), ERR_INSUFFICIENT); // disp16 cut short
    EXPECT_EQ(x86({ 0x67, 0x8D, 0x80, 1, 2 }), 5u);
    EXPECT_EQ(x86({ 0x8D, 0x80, 1, 2, 3 }), ERR_INSUFFICIENT); // disp32 cut short
    EXPECT_EQ(x86({ 0x8D, 0x80, 1, 2, 3, 4 }), 6u);
}

TEST(BoundaryTest32, ThreeByte_escapes_are_undefined_in_ia32)
{
    /* The IA-32 map has no 0F 38 / 0F 3A three-byte escapes; they stay 2-byte
     * opcodes with a ModRM, which is what the baseline did. */
    EXPECT_EQ(x86({ 0x0F, 0x38, 0xF0, 0xF8 }), ERR_UNDEFINED);
    EXPECT_EQ(x86({ 0x0F, 0x3A, 0x0F }), ERR_UNDEFINED);
}

TEST(BoundaryTest, Empty_and_null_buffers)
{
    EXPECT_EQ(lendiza::detail::amd64traits::ldiza(nullptr, 5), ERR_INSUFFICIENT);
    EXPECT_EQ(lendiza::detail::x86traits::ldiza(nullptr, 5), ERR_INSUFFICIENT);
    static const uint8_t one = 0x90;
    EXPECT_EQ(lendiza::detail::amd64traits::ldiza(&one, 0), ERR_INSUFFICIENT);
    EXPECT_EQ(lendiza::disasm_x64(&one, 1), 1u);
}

TEST(BoundaryTest, Result_is_stable_across_repeated_calls)
{
    /* Guards against any leftover cross-call state: the same bytes must always
     * produce the same answer, no matter what was decoded before them. */
    static const uint8_t seq[] = { 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8, 0x66, 0xB8, 1, 2, 0xF6, 0x05 };
    const size_t lone = lendiza::detail::amd64traits::ldiza(seq, sizeof(seq));
    for (size_t i = 0; i < sizeof(seq); ++i) {
        EXPECT_EQ(lendiza::detail::amd64traits::ldiza(seq + i, sizeof(seq) - i),
                  lendiza::detail::amd64traits::ldiza(seq + i, sizeof(seq) - i));
    }
    EXPECT_EQ(lone, 10u); // 48 B8 imm64 - the REX never leaks into the next call
    EXPECT_EQ(lendiza::detail::amd64traits::ldiza(seq + 1, sizeof(seq) - 1), 5u);
    EXPECT_EQ(lendiza::detail::amd64traits::ldiza(seq + 10, sizeof(seq) - 10), 4u);
}
