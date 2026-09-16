// dump_group_baseline.cpp - record the decoder's answers over the exhaustive
// group/x87 sweep in lendiza_group_sweep.h, BEFORE the table refactor.
//
//   cmake --build build --target lendiza_dump_group_baseline
//   build/lendiza_dump_group_baseline.exe tests/data/group_before.bin [--force]
//
// test_group_oracle.cpp replays the same sweep and requires zero differences.
// The committed baseline IS that oracle, so this tool refuses to overwrite an
// existing file: re-recording needs a reviewed, intended behaviour change,
// which is what --force states out loud.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <vector>

#include "lendiza.hpp"
#include "lendiza_group_sweep.h"

namespace {

struct Header {
    char magic[4];
    uint32_t version;
    uint32_t count;
};

std::vector<uint8_t> g_results;

void sink(const uint8_t* bytes, const uint8_t len, const unsigned mode, void*)
{
    const std::span<const uint8_t> buf { bytes, len };
    const size_t r = mode == 64
                         ? lendiza::ldiza_x86<64> {}.ldisasm(buf)
                         : lendiza::ldiza_x86<32> {}.ldisasm(buf);
    g_results.push_back(static_cast<uint8_t>(r));
}

} // namespace

int main(int argc, char** argv)
{
    bool force = false;
    const char* out_path = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--force") == 0) {
            force = true;
        } else if (out_path == nullptr) {
            out_path = argv[i];
        } else {
            std::fprintf(stderr, "usage: %s <output-file> [--force]\n", argv[0]);
            return 2;
        }
    }
    if (out_path == nullptr) {
        std::fprintf(stderr, "usage: %s <output-file> [--force]\n", argv[0]);
        return 2;
    }

    /* Guard against silently replacing the committed oracle: an existing file
     * is only overwritten when --force states that intent out loud. */
    if (!force) {
        if (std::FILE* probe = std::fopen(out_path, "rb")) {
            std::fclose(probe);
            std::fprintf(stderr,
                         "%s already exists - refusing to overwrite.  The group "
                         "baseline is a reviewed oracle; re-record it only for an "
                         "intended, approved behaviour change, then pass --force.\n",
                         out_path);
            return 1;
        }
    }

    lzsw::for_each(&sink, nullptr);

    Header h { { 'L', 'G', 'T', 'B' }, 1u, static_cast<uint32_t>(g_results.size()) };
    std::FILE* f = std::fopen(out_path, "wb");
    if (f == nullptr) {
        std::fprintf(stderr, "cannot write %s\n", out_path);
        return 1;
    }
    if (std::fwrite(&h, sizeof(h), 1, f) != 1) {
        std::perror(out_path);
        std::fclose(f);
        return 1;
    }
    if (std::fwrite(g_results.data(), 1, g_results.size(), f) != g_results.size()) {
        std::perror(out_path);
        std::fclose(f);
        return 1;
    }
    std::fclose(f);
    std::printf("recorded %zu cases\n", g_results.size());
    return 0;
}
