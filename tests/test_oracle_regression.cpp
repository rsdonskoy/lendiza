// test_oracle_regression.cpp
// Replays the committed behaviour baseline against the current decoder.
//
// tests/data/oracle_before.bin was recorded from the pre-kernel-port decoder
// over the corpus in lendiza_corpus.h (718297 cases per mode).  The refactor was
// expected to change behaviour in exactly six reviewed ways, each encoded in
// lendiza_baseline.h:
//
//   prefix         prefix bytes now belong to the instruction they precede
//   truncated      a window too small to hold the encoding no longer yields a
//                  guessed length (it used to read past the end)
//   group2-count   0F 00 / 0F 01 memory forms count ModRM + SIB + displacement
//   group-disp     F6/F7/FE/FF/C6/C7 count SIB and displacement
//   error-refined  0F family: a demanded byte that was not needed, or a length
//                  where the old table only said "invalid"
//
// Any difference outside those categories fails this suite.  Re-record the
// baseline only when a behaviour change is intended and reviewed:
//   g++ -std=c++20 -I../include -I.. tools/dump_baseline.cpp -o dump && ./dump data/oracle_before.bin
//
// Suite: OracleTest.

#include <cstdint>
#include <cstdio>
#include <span>
#include <vector>

#include "lendiza_baseline.h"
#include "lendiza_test.h"
#include "lendiza.hpp"

#ifndef LENDIZA_ORACLE_PATH
#define LENDIZA_ORACLE_PATH "data/oracle_before.bin"
#endif

namespace {

std::vector<lzc::Case> g_cases;
std::vector<uint8_t> g_now32;
std::vector<uint8_t> g_now64;

void sink32(const lzc::Case& c, void*)
{
    const std::span<const uint8_t> buf { c.bytes, c.len };
    g_now32.push_back(static_cast<uint8_t>(lendiza::ldiza_x86<32> {}.ldisasm(buf)));
}

void sink64(const lzc::Case& c, void*)
{
    const std::span<const uint8_t> buf { c.bytes, c.len };
    g_now64.push_back(static_cast<uint8_t>(lendiza::ldiza_x86<64> {}.ldisasm(buf)));
}

struct Loaded {
    std::vector<uint8_t> before32;
    std::vector<uint8_t> before64;
    bool ok = false;
};

const Loaded& loaded()
{
    static Loaded l = [] {
        Loaded r;
        r.ok = lzbase::load(LENDIZA_ORACLE_PATH, r.before32, r.before64);
        if (r.ok) {
            lzc::Corpus c32 { &sink32, nullptr };
            c32.run();
            lzc::Corpus c64 { &sink64, nullptr };
            c64.run();
            std::vector<lzc::Case> cases;
            lzc::Corpus cc { &lzbase::collect, &cases };
            cc.run();
            g_cases = std::move(cases);
        }
        return r;
    }();
    return l;
}

} // namespace

TEST(OracleTest, BaselineFileIsAvailable)
{
    if (!loaded().ok) {
        std::printf("    baseline %s missing - build it with tools/dump_baseline\n",
                    LENDIZA_ORACLE_PATH);
    }
    EXPECT_TRUE(loaded().ok);
}

TEST(OracleTest, CorpusReplayMatchesTheRecordedCaseCount)
{
    if (!loaded().ok) {
        return;
    }
    EXPECT_EQ(g_cases.size(), loaded().before32.size());
    EXPECT_EQ(g_now32.size(), loaded().before32.size());
    EXPECT_EQ(g_now64.size(), loaded().before32.size());
}

TEST(OracleTest, NoUnexpectedDifferencesIn32BitMode)
{
    if (!loaded().ok) {
        return;
    }
    lzbase::Result r;
    lzbase::accumulate(r, g_cases, loaded().before32, g_now32, false, 12);
    for (const auto& s : r.unexpected_samples) {
        std::printf("    [x86] %s\n", s.c_str());
    }
    std::printf("    [x86] diffs=%zu prefix=%zu truncated=%zu group2=%zu group-disp=%zu ia32-row-a=%zu "
                "error-refined=%zu unexpected=%zu\n",
                r.differences, r.counts[lzbase::cat_prefix], r.counts[lzbase::cat_truncated],
                r.counts[lzbase::cat_group2_count], r.counts[lzbase::cat_group_disp], r.counts[lzbase::cat_ia32_row_a],
                r.counts[lzbase::cat_error_refined], r.counts[lzbase::cat_unexpected]);
    EXPECT_EQ(r.counts[lzbase::cat_unexpected], 0u);
    EXPECT_TRUE(r.counts[lzbase::cat_ia32_row_a] > 0u); /* the IA-32 row A fix must show up */
}

TEST(OracleTest, NoUnexpectedDifferencesIn64BitMode)
{
    if (!loaded().ok) {
        return;
    }
    lzbase::Result r;
    lzbase::accumulate(r, g_cases, loaded().before64, g_now64, true, 12);
    for (const auto& s : r.unexpected_samples) {
        std::printf("    [x64] %s\n", s.c_str());
    }
    std::printf("    [x64] diffs=%zu prefix=%zu truncated=%zu group2=%zu group-disp=%zu ia32-row-a=%zu "
                "error-refined=%zu unexpected=%zu\n",
                r.differences, r.counts[lzbase::cat_prefix], r.counts[lzbase::cat_truncated],
                r.counts[lzbase::cat_group2_count], r.counts[lzbase::cat_group_disp], r.counts[lzbase::cat_ia32_row_a],
                r.counts[lzbase::cat_error_refined], r.counts[lzbase::cat_unexpected]);
    EXPECT_EQ(r.counts[lzbase::cat_unexpected], 0u);
    EXPECT_EQ(r.counts[lzbase::cat_ia32_row_a], 0u); /* the row A fix is IA-32 only */
}

TEST(OracleTest, UnprefixedCompleteEncodingsAreUntouched)
{
    /* The narrowest reading of "no regressions": no prefix byte in sight, the
     * baseline had enough bytes to read the whole encoding, and the form is not
     * one of the group handlers deliberately fixed in this refactor.  Everything
     * else must be byte-for-byte identical. */
    if (!loaded().ok) {
        return;
    }

    size_t checked = 0;
    size_t skipped_group = 0;
    size_t violations = 0;

    for (size_t i = 0; i < g_cases.size(); ++i) {
        const lzc::Case& c = g_cases[i];
        const uint8_t before = loaded().before64[i];

        if (c.len < 3 || lzbase::is_prefix_byte(c.bytes[0], true) || before >= 0xE0 ||
            static_cast<size_t>(before) > c.len) {
            continue;
        }
        switch (c.bytes[0]) {
        case 0xF6: case 0xF7: case 0xFE: case 0xFF: case 0xC6: case 0xC7:
            ++skipped_group;
            continue;
        default:
            break;
        }
        if (c.bytes[0] == 0x0F && c.len > 1 && (c.bytes[1] == 0x00 || c.bytes[1] == 0x01)) {
            ++skipped_group;
            continue;
        }

        ++checked;
        if (g_now64[i] != before) {
            ++violations;
            if (violations < 6) {
                std::printf("    [%zu] len=%u before=%u now=%u\n", i, c.len, before, g_now64[i]);
            }
        }
    }
    std::printf("    unprefixed complete cases checked: %zu (skipped %zu group forms fixed on purpose)\n",
                checked, skipped_group);
    EXPECT_TRUE(checked > 50000);
    EXPECT_EQ(violations, 0u);
}

TEST(OracleTest, LinearWalkOfRealCodeCoversAtLeastAsMuchAsBefore)
{
    /* tests/data/walk_before.txt holds "mode stop steps" recorded by walking the
     * real-code corpus with the pre-refactor decoder.  A length disassembler that
     * stops earlier than before has derailed, so coverage may only grow; and
     * because prefixes now belong to the instruction they precede, the number of
     * steps over the same bytes must strictly fall. */
#ifndef LENDIZA_WALK_PATH
#define LENDIZA_WALK_PATH "data/walk_before.txt"
#endif
    std::FILE* f = std::fopen(LENDIZA_WALK_PATH, "r");
    if (f == nullptr) {
        std::printf("    missing %s\n", LENDIZA_WALK_PATH);
        EXPECT_TRUE(false);
        return;
    }

    size_t n = 0;
    const uint8_t* code = lzc::shellcode(n);

    int mode = 0;
    unsigned long stop_before = 0;
    unsigned long steps_before = 0;
    while (std::fscanf(f, "%d %lu %lu", &mode, &stop_before, &steps_before) == 3) {
        size_t off = 0;
        size_t steps = 0;
        while (off < n) {
            const size_t window = (n - off < LDZ_MAX_INSNS_LEN) ? (n - off) : LDZ_MAX_INSNS_LEN;
            const size_t len = (mode == 32)
                                   ? lendiza::detail::x86traits::ldiza(code + off, window)
                                   : lendiza::detail::amd64traits::ldiza(code + off, window);
            if (len >= 0xE0 || len == 0) {
                break;
            }
            off += len;
            ++steps;
        }
        std::printf("    mode=%d stop=%zu (was %zu) steps=%zu (was %zu)\n", mode, off,
                    static_cast<size_t>(stop_before), steps, static_cast<size_t>(steps_before));
        EXPECT_TRUE(off >= stop_before);
        EXPECT_TRUE(steps < steps_before);
        /* The corpus is 32-bit code, so the 32-bit walk must reach the padding. */
        if (mode == 32) {
            EXPECT_TRUE(off > 3000u);
        }
    }
    std::fclose(f);
}

TEST(OracleTest, LengthDoesNotDependOnExtraTrailingBytes)
{
    /* A driver hands over whatever is readable, so widening the window must never
     * change an answer that was already complete. */
    size_t n = 0;
    const uint8_t* code = lzc::shellcode(n);

    size_t compared = 0;
    size_t violations = 0;
    for (size_t off = 0; off + 2 < n; ++off) {
        for (size_t w = 2; w + off <= n && w <= LDZ_MAX_INSNS_LEN; ++w) {
            const size_t short_len = lendiza::detail::amd64traits::ldiza(code + off, w);
            if (short_len >= 0xE0) {
                continue;
            }
            const size_t wide_len = lendiza::detail::amd64traits::ldiza(code + off, w + 1);
            ++compared;
            if (wide_len != short_len) {
                ++violations;
                if (violations < 6) {
                    std::printf("    off=%zu window=%zu -> %zu but %zu -> %zu\n", off, w, short_len,
                                w + 1, wide_len);
                }
            }
        }
    }
    std::printf("    window comparisons: %zu\n", compared);
    EXPECT_TRUE(compared > 10000u);
    EXPECT_EQ(violations, 0u);
}
