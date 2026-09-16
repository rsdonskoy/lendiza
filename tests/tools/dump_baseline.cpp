// dump_baseline.cpp - records the decoder's answer for every corpus case.
//
// Run it against a given checkout and commit the output as the oracle baseline;
// test_oracle_regression.cpp then replays the same corpus and reports every
// difference, so behaviour changes are reviewable data rather than surprises.
//
//   g++ -std=c++20 -I../include -I.. dump_baseline.cpp -o dump_baseline
//   ./dump_baseline ../data/oracle_before.txt
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <vector>

#include "lendiza.hpp"
#include "lendiza_corpus.h"

namespace {

struct Header {
    char magic[4];
    uint32_t version;
    uint32_t count;
};

std::vector<uint8_t> g_results;

void sink32(const lzc::Case& c, void*)
{
    const std::span<const uint8_t> buf { c.bytes, c.len };
    const size_t len = lendiza::ldiza_x86<32> {}.ldisasm(buf);
    g_results.push_back(static_cast<uint8_t>(len));
}

std::vector<uint8_t> g_results64;

void sink64(const lzc::Case& c, void*)
{
    const std::span<const uint8_t> buf { c.bytes, c.len };
    const size_t len = lendiza::ldiza_x86<64> {}.ldisasm(buf);
    g_results64.push_back(static_cast<uint8_t>(len));
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <output-file>\n", argv[0]);
        return 2;
    }

    {
        lzc::Corpus c32 { &sink32, nullptr };
        c32.run();
        std::printf("cases per mode: %zu\n", c32.count());
    }
    {
        lzc::Corpus c64 { &sink64, nullptr };
        c64.run();
    }

    if (g_results.size() != g_results64.size()) {
        std::fprintf(stderr, "corpus drift between modes\n");
        return 1;
    }

    Header h {};
    std::memcpy(h.magic, "LDZB", 4);
    h.version = 1;
    h.count = static_cast<uint32_t>(g_results.size());

    std::FILE* f = std::fopen(argv[1], "wb");
    if (f == nullptr) {
        std::fprintf(stderr, "cannot open %s\n", argv[1]);
        return 1;
    }
    std::fwrite(&h, sizeof(h), 1, f);
    std::fwrite(g_results.data(), 1, g_results.size(), f);
    std::fwrite(g_results64.data(), 1, g_results64.size(), f);
    std::fclose(f);

    std::printf("wrote %s (%u cases/mode)\n", argv[1], h.count);
    return 0;
}
