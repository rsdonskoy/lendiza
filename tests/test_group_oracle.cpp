// test_group_oracle.cpp
// Replays the exhaustive group/x87 sweep against tests/data/group_before.bin
// (recorded from the pre-table-refactor decoder by tools/dump_group_baseline).
// The refactor is behavior-preserving by contract, so ANY difference fails.
//
// Suite: GroupOracleTest.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#include "lendiza.hpp"
#include "lendiza_group_sweep.h"
#include "lendiza_test.h"

#ifndef LENDIZA_GROUP_BASELINE_PATH
#define LENDIZA_GROUP_BASELINE_PATH "data/group_before.bin"
#endif

namespace {

struct Header {
    char magic[4];
    uint32_t version;
    uint32_t count;
};

std::vector<uint8_t> g_before;
size_t g_index = 0;
size_t g_diffs = 0;
std::vector<std::string> g_samples;

bool load()
{
    std::FILE* f = std::fopen(LENDIZA_GROUP_BASELINE_PATH, "rb");
    if (f == nullptr) {
        return false;
    }
    Header h {};
    if (std::fread(&h, sizeof(h), 1, f) != 1 || std::memcmp(h.magic, "LGTB", 4) != 0 ||
        h.version != 1u) {
        std::fclose(f);
        return false;
    }
    g_before.resize(h.count);
    const size_t got = std::fread(g_before.data(), 1, g_before.size(), f);
    std::fclose(f);
    return got == g_before.size();
}

const bool g_ok = load();

void sink(const uint8_t* bytes, const uint8_t len, const unsigned mode, void*)
{
    const std::span<const uint8_t> buf { bytes, len };
    const size_t now = mode == 64
                           ? lendiza::ldiza_x86<64> {}.ldisasm(buf)
                           : lendiza::ldiza_x86<32> {}.ldisasm(buf);
    if (g_index < g_before.size() && static_cast<uint8_t>(now) != g_before[g_index]) {
        ++g_diffs;
        if (g_samples.size() < 12) {
            std::string s;
            for (uint8_t i = 0; i < len; ++i) {
                char hex[4];
                std::snprintf(hex, sizeof(hex), "%02X ", bytes[i]);
                s += hex;
            }
            char tail[64];
            std::snprintf(tail, sizeof(tail), "| mode=%u before=0x%02X now=0x%02X", mode,
                          g_before[g_index], static_cast<uint8_t>(now));
            g_samples.push_back(s + tail);
        }
    }
    ++g_index;
}

} // namespace

TEST(GroupOracleTest, BaselineFileIsAvailable)
{
    if (!g_ok) {
        std::printf("    baseline %s missing - build tools/dump_group_baseline first\n",
                    LENDIZA_GROUP_BASELINE_PATH);
    }
    EXPECT_TRUE(g_ok);
}

TEST(GroupOracleTest, ZeroDifferencesAgainstPreRefactorBaseline)
{
    if (!g_ok) {
        return;
    }
    g_index = 0;
    g_diffs = 0;
    g_samples.clear();
    lzsw::for_each(&sink, nullptr);
    for (const auto& s : g_samples) {
        std::printf("    %s\n", s.c_str());
    }
    std::printf("    replayed=%zu recorded=%zu diffs=%zu\n", g_index, g_before.size(), g_diffs);
    EXPECT_EQ(g_index, g_before.size());
    EXPECT_EQ(g_diffs, 0u);
}
