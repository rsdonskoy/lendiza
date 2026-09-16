// test_group_tables.cpp
// Compile-time value-domain check for the 34 dispatch tables: every entry must
// be one of the encodings defined in the spec (a hand-pasted typo must not
// survive as a silent UNKNOWN_ERROR at runtime).
//
// Also a compile-time domain check for the 0xEF / 0xEE sentinels of the four
// main opcode-to-length maps: a sentinel may only sit where the decode dispatch
// can turn it into an in-range pick-table index.
//
// Suite: GroupTableTest.
#include <cstdint>

#include "lendiza_common.hpp"
#include "lendiza_x86_tables.hpp"
#include "lendiza_amd64_tables.hpp"
#include "lendiza_test.h"

namespace lzgt {

using lendiza::detail::kGrpFixedBase;
using lendiza::detail::kGrpTailFlag;

/* Accepted encoding domain, spec section 2.  Note how much of it the runtime
 * oracle actually proves: the committed baseline tests/data/group_before.bin
 * only exercises six distinct values (0x00, 0x01, 0x83, 0xA3, 0xA4, 0xE1), so
 * every other encoding admitted below is vocabulary the spec reserves, not a
 * shape GroupOracleTest has replayed. */
constexpr bool encoding_ok(const uint8_t v)
{
    if (v >= 0xE0 && v <= 0xE2) { return true; }                       // LDZ_ERR codes
    if (v > kGrpFixedBase && v < kGrpFixedBase + 0x10) {                // 0xA0|n, n in [2,15]
        const uint8_t n = static_cast<uint8_t>(v & 0x0F);
        return n >= 2 && n <= 15;
    }
    if (v == (kGrpTailFlag | 3)) { return true; }                       // tail_iz, the only resized tail in use
    return v == 0x00 || v == 0x01 || v == 0x02 || v == 0x04 || v == 0x08;
}

template <size_t N> constexpr bool table_ok(const uint8_t (&t)[N])
{
    for (const uint8_t v : t) {
        if (!encoding_ok(v)) { return false; }
    }
    return true;
}

/* ---------------------------------------------------------------------------
 * Main-map sentinel guards.
 *
 * The decoders turn a mapped value straight into a pick-table index: IA-32
 * answers `mapped == 0xEF` with `kIa32LengthTable_x87[opc - 0xD8]` (lendiza_x86.hpp,
 * decode) and amd64 mirrors that same row arithmetic with `kLengthTable_x87[opc - 0xD8]`
 * (lendiza_amd64.hpp:507); both architectures answer `mapped == 0xEE` by
 * handing the opcode byte to the group picks (pick_grp1_table /
 * pick_grp2_table).  Those group picks fall back to nullptr, but the x87 row
 * index is unchecked, so a 0xEF sitting outside the 0xD8-0xDF cells would read
 * past the end of the 2 KiB x87 table.  The asserts below pin every sentinel
 * to exactly the cells the dispatch code can service, so editing a main map
 * cannot turn into an out-of-bounds index.
 *
 * Out of scope on purpose: the amd64 maps carry further sentinels the dispatch
 * never indexes with - 0xE8/0xEA escape to the three-byte opcode families and
 * live in the two-byte map only (kLengthTable_2byte_opc[0x38] / [0x3a]), and
 * 0xFF marks undefined cells in both amd64 maps.  None of them names a table,
 * so none needs a domain guard here.
 */

/* Map sentinels; values must match the dispatch cases described above. */
constexpr uint8_t kMapEscX87 = 0xEF; /* escape to the floating-point tables */
constexpr uint8_t kMapOpExt = 0xEE;  /* opcode carries an opcode extension */

/* Opcodes the 0xEF escape may name: both x87 tables are [8][256] and are
 * indexed [opcode - 0xD8], so only 0xD8..0xDF address a row. */
constexpr uint8_t kX87Opcodes[] = {0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF};

/* One-byte opcode-extension groups the 0xEE sentinel may name; identical set
 * on both architectures. */
constexpr uint8_t kGrp1Opcodes[] = {0xC6, 0xC7, 0xF6, 0xF7, 0xFE, 0xFF};

/* Two-byte (0F xx) opcode-extension groups the 0xEE sentinel may name. */
constexpr uint8_t kGrp2Opcodes[] = {0x00, 0x01, 0x18, 0x71, 0x72, 0x73, 0xAE, 0xB9, 0xBA, 0xC7};

template <size_t M> constexpr bool is_member(const size_t v, const uint8_t (&set)[M])
{
    for (const uint8_t s : set) {
        if (static_cast<size_t>(s) == v) { return true; }
    }
    return false;
}

/* True when every index of `map` whose entry equals `sentinel` is listed in
 * `allowed`, i.e. when the dispatch reading that sentinel can reach a table. */
template <size_t N, size_t M>
constexpr bool sentinel_positions_ok(const uint8_t (&map)[N], const uint8_t sentinel,
                                     const uint8_t (&allowed)[M])
{
    for (size_t i = 0; i < N; ++i) {
        if (map[i] == sentinel && !is_member(i, allowed)) { return false; }
    }
    return true;
}

} // namespace lzgt

// All 32 one- and two-byte group tables (16 per architecture) plus the two 8x256 x87 tables.
namespace {

constexpr const uint8_t (*const x86_grp1[6])[256] = {
    &lendiza::detail::kIa32LengthTable_opcode_extension_0xC6, &lendiza::detail::kIa32LengthTable_opcode_extension_0xC7,
    &lendiza::detail::kIa32LengthTable_opcode_extension_0xF6, &lendiza::detail::kIa32LengthTable_opcode_extension_0xF7,
    &lendiza::detail::kIa32LengthTable_opcode_extension_0xFE, &lendiza::detail::kIa32LengthTable_opcode_extension_0xFF,
};
constexpr const uint8_t (*const x86_grp2[10])[256] = {
    &lendiza::detail::kIa32LengthTable_0F_00, &lendiza::detail::kIa32LengthTable_0F_01,
    &lendiza::detail::kIa32LengthTable_0F_18, &lendiza::detail::kIa32LengthTable_0F_71,
    &lendiza::detail::kIa32LengthTable_0F_72, &lendiza::detail::kIa32LengthTable_0F_73,
    &lendiza::detail::kIa32LengthTable_0F_AE, &lendiza::detail::kIa32LengthTable_0F_B9,
    &lendiza::detail::kIa32LengthTable_0F_BA, &lendiza::detail::kIa32LengthTable_0F_C7,
};
constexpr const uint8_t (*const amd_grp1[6])[256] = {
    &lendiza::detail::kLengthTable_opcode_extension_0xC6, &lendiza::detail::kLengthTable_opcode_extension_0xC7,
    &lendiza::detail::kLengthTable_opcode_extension_0xF6, &lendiza::detail::kLengthTable_opcode_extension_0xF7,
    &lendiza::detail::kLengthTable_opcode_extension_0xFE, &lendiza::detail::kLengthTable_opcode_extension_0xFF,
};
constexpr const uint8_t (*const amd_grp2[10])[256] = {
    &lendiza::detail::kLengthTable_0F_00, &lendiza::detail::kLengthTable_0F_01,
    &lendiza::detail::kLengthTable_0F_18, &lendiza::detail::kLengthTable_0F_71,
    &lendiza::detail::kLengthTable_0F_72, &lendiza::detail::kLengthTable_0F_73,
    &lendiza::detail::kLengthTable_0F_AE, &lendiza::detail::kLengthTable_0F_B9,
    &lendiza::detail::kLengthTable_0F_BA, &lendiza::detail::kLengthTable_0F_C7,
};

constexpr bool all_ok()
{
    for (const auto* t : x86_grp1) { if (!lzgt::table_ok(*t)) { return false; } }
    for (const auto* t : x86_grp2) { if (!lzgt::table_ok(*t)) { return false; } }
    for (const auto* t : amd_grp1) { if (!lzgt::table_ok(*t)) { return false; } }
    for (const auto* t : amd_grp2) { if (!lzgt::table_ok(*t)) { return false; } }
    for (const auto& row : lendiza::detail::kIa32LengthTable_x87) { if (!lzgt::table_ok(row)) { return false; } }
    for (const auto& row : lendiza::detail::kLengthTable_x87) { if (!lzgt::table_ok(row)) { return false; } }
    return true;
}

static_assert(all_ok(), "group/x87 table entry outside the documented encoding");

/* The 0xEF / 0xEE sentinels of the four main opcode->length maps stay inside
 * the index domain their dispatch site can service. */
static_assert(lzgt::sentinel_positions_ok(lendiza::detail::kIa32LengthTable_1byte_opc,
                                          lzgt::kMapEscX87, lzgt::kX87Opcodes) &&
                  lzgt::sentinel_positions_ok(lendiza::detail::kLengthTable_1byte_opc,
                                              lzgt::kMapEscX87, lzgt::kX87Opcodes),
              "0xEF map sentinel escaped the pick-table domain -> OOB index in decode dispatch");

static_assert(lzgt::sentinel_positions_ok(lendiza::detail::kIa32LengthTable_1byte_opc,
                                          lzgt::kMapOpExt, lzgt::kGrp1Opcodes) &&
                  lzgt::sentinel_positions_ok(lendiza::detail::kLengthTable_1byte_opc,
                                              lzgt::kMapOpExt, lzgt::kGrp1Opcodes),
              "1-byte 0xEE map sentinel escaped the pick-table domain -> OOB index in decode dispatch");

static_assert(lzgt::sentinel_positions_ok(lendiza::detail::kIa32LengthTable_2byte_opc,
                                          lzgt::kMapOpExt, lzgt::kGrp2Opcodes) &&
                  lzgt::sentinel_positions_ok(lendiza::detail::kLengthTable_2byte_opc,
                                              lzgt::kMapOpExt, lzgt::kGrp2Opcodes),
              "2-byte 0xEE map sentinel escaped the pick-table domain -> OOB index in decode dispatch");

} // namespace

TEST(GroupTableTest, EncodingsAreWellFormedAtCompileTime)
{
    EXPECT_TRUE(all_ok()); /* the static_assert above is the real gate */
}
