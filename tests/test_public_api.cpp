// test_public_api.cpp
// Coverage for every entry point a caller can reach: the std::span overloads, the
// polymorphic lendiza::ldiza interface, the inline core helpers, and the extern "C"
// ABI in lendiza_export.h.
//
// The refactor moved all decoding into the pointer+length core; what is asserted
// here is that the hosted surface still behaves as one consistent function - same
// answer through every entry point, same error values, no shared state between
// calls - and that the documented error codes still line up between C and C++.
//
// Suite: PublicApiTest.

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "lendiza.hpp"
#include "lendiza_core.hpp"
#include "lendiza_decode_helpers.h"
#include "lendiza_export.h"
#include "lendiza_test.h"

using namespace lzdh;

namespace {

/* Small, hand-checked set: each entry is (bytes, x64 length, x86 length). */
struct Sample {
    std::array<uint8_t, 11> bytes;
    size_t size;
    size_t len64;
    size_t len32;
};

const std::vector<Sample>& samples()
{
    static const std::vector<Sample> s = {
        { { 0x90 }, 1, 1, 1 },                                                     // NOP
        { { 0xC3 }, 1, 1, 1 },                                                     // RET
        { { 0xB8, 1, 2, 3, 4 }, 5, 5, 5 },                                         // MOV eAX, imm32
        { { 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 }, 10, 10, 1 },                     // REX.W MOV / DEC eax
        { { 0x66, 0xB8, 1, 2 }, 4, 4, 4 },                                         // MOV ax, imm16
        { { 0x55 }, 1, 1, 1 },                                                     // PUSH rbp / push ebp
        { { 0x8B, 0x45, 0x08 }, 3, 3, 3 },                                         // MOV eax, [rbp+8]/[ebp+8]
        { { 0x0F, 0xB6, 0xC0 }, 3, 3, 3 },                                         // MOVZX
        { { 0xE8, 1, 2, 3, 4 }, 5, 5, 5 },                                         // CALL rel32
        { { 0xFF, 0x25, 1, 2, 3, 4 }, 6, 6, 6 },                                   // JMP [rip+disp32] / JMP [abs32]
        { { 0x65, 0x48, 0x8B, 0x04, 0x25, 0x10, 0, 0, 0 }, 9, 9, 2 },              // GS MOV rax, gs:[0x10]
    };
    return s;
}

} // namespace

TEST(PublicApiTest, SpanCallOperatorMatchesLdisasm)
{
    for (const Sample& s : samples()) {
        const std::span<const uint8_t> buf { s.bytes.data(), s.size };
        const lendiza::ldiza_x86<64> d64;
        const lendiza::ldiza_x86<32> d32;
        EXPECT_EQ(d64(buf), d64.ldisasm(buf));
        EXPECT_EQ(d32(buf), d32.ldisasm(buf));
    }
}

TEST(PublicApiTest, DocumentedLengthsThroughTheSpanInterface)
{
    for (const Sample& s : samples()) {
        const std::span<const uint8_t> buf { s.bytes.data(), s.size };
        EXPECT_EQ(lendiza::ldiza_x86<64> {}(buf), s.len64);
        EXPECT_EQ(lendiza::ldiza_x86<32> {}(buf), s.len32);
    }
}

TEST(PublicApiTest, PolymorphicBaseDispatchesToTheRightMode)
{
    const lendiza::ldiza_x86<64> six;
    const lendiza::ldiza_x86<32> thirty;
    const std::array<uint8_t, 10> code = { 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 };
    const std::span<const uint8_t> buf { code };

    const lendiza::ldiza& as_base64 = six;
    const lendiza::ldiza& as_base32 = thirty;
    EXPECT_EQ(as_base64.ldisasm(buf), 10u); // REX.W MOV rax, imm64
    EXPECT_EQ(as_base32.ldisasm(buf), 1u);  // DEC eax
    EXPECT_EQ(six(buf), as_base64.ldisasm(buf));
}

TEST(PublicApiTest, EmptyAndNullBuffers)
{
    const std::span<const uint8_t> empty;
    EXPECT_EQ(lendiza::ldiza_x86<64> {}(empty), ERR_INSUFFICIENT);
    EXPECT_EQ(lendiza::ldiza_x86<32> {}(empty), ERR_INSUFFICIENT);
    EXPECT_EQ(lendiza_disasm_x64(nullptr, 8), ERR_INSUFFICIENT);
    EXPECT_EQ(lendiza_disasm_x86(nullptr, 8), ERR_INSUFFICIENT);
    static const uint8_t one = 0x90;
    EXPECT_EQ(lendiza_disasm_x64(&one, 0), ERR_INSUFFICIENT);
}

TEST(PublicApiTest, C_ABIAgreesWithTheCppInterface)
{
    for (const Sample& s : samples()) {
        EXPECT_EQ(lendiza_disasm_x64(s.bytes.data(), s.size), lendiza::disasm_x64(s.bytes.data(), s.size));
        EXPECT_EQ(lendiza_disasm_x86(s.bytes.data(), s.size), lendiza::disasm_x86(s.bytes.data(), s.size));
    }
}

TEST(PublicApiTest, ErrorCodesMatchBetweenCAndCxx)
{
    EXPECT_EQ(static_cast<size_t>(lendiza::ErrorCode::INSUFFICIENT_BUFFER),
              static_cast<size_t>(LENDIZA_ERR_INSUFFICIENT_BUFFER));
    EXPECT_EQ(static_cast<size_t>(lendiza::ErrorCode::UNDEFINED_INSTRUCTION),
              static_cast<size_t>(LENDIZA_ERR_UNDEFINED_INSTRUCTION));
    EXPECT_EQ(static_cast<size_t>(lendiza::ErrorCode::UNKNOWN_ERROR),
              static_cast<size_t>(LENDIZA_ERR_UNKNOWN));
    EXPECT_EQ(static_cast<size_t>(LDZ_MAX_INSNS_LEN), static_cast<size_t>(LENDIZA_MAX_INSNS_LEN));
}

TEST(PublicApiTest, IsErrorPredicateSeparatesLengthsFromErrors)
{
    EXPECT_TRUE(lendiza::is_error(ERR_INSUFFICIENT));
    EXPECT_TRUE(lendiza::is_error(ERR_UNDEFINED));
    EXPECT_TRUE(lendiza::is_error(ERR_UNKNOWN));
    EXPECT_TRUE(!lendiza::is_error(1));
    EXPECT_TRUE(!lendiza::is_error(LDZ_MAX_INSNS_LEN));
}

TEST(PublicApiTest, NoStateCarriesBetweenCalls)
{
    /* Decoding a REX-prefixed instruction must not influence the next call - the
     * old implementation kept the REX byte in a static member. */
    static const std::array<uint8_t, 10> rex_mov = { 0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8 };
    static const std::array<uint8_t, 5> plain_mov = { 0xB8, 1, 2, 3, 4 };
    const std::span<const uint8_t> rex_buf { rex_mov };
    const std::span<const uint8_t> plain_buf { plain_mov };

    const lendiza::ldiza_x86<64> d;
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(d(rex_buf), 10u);
        EXPECT_EQ(d(plain_buf), 5u);
        EXPECT_EQ(d(rex_buf), 10u);
    }
}

TEST(PublicApiTest, WorksFromAVectorBufferAndAnAlignedStackArray)
{
    std::vector<uint8_t> v = { 0x48, 0x83, 0xEC, 0x20 }; // SUB rsp, 0x20
    EXPECT_EQ(lendiza::disasm_x64(v.data(), v.size()), 4u);

    alignas(8) const uint8_t stack_buf[4] = { 0x66, 0x0F, 0x1F, 0x00 }; // nop word ptr [rax]
    EXPECT_EQ(lendiza::disasm_x64(stack_buf, sizeof(stack_buf)), 4u);
}
