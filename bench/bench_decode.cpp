/* Self-contained linear-sweep benchmark for the lendiza length decoder.
 *
 * Two modes:
 *
 *   bench_decode.exe <file.text.bin>... [--arch x64|x86] [--iters N]
 *       Times a linear sweep over each corpus and reports MiB/s and
 *       Minstructions/s.  With no file arguments a deterministic 8 MiB
 *       synthetic corpus is generated in memory.
 *
 *   bench_decode.exe --dump <out.bin> <file>... [--arch x64|x86]
 *       Runs the sweep once per corpus and writes one result byte per
 *       offset: the decoded length (1..15) for a successful decode, or
 *       0xF0|code for an error (0xE0/0xE1/0xE2 -> low nibble).  Comparing
 *       dump files before and after a change is the differential check.
 *
 * The sweep advances by the decoded length on success and by one byte on
 * an error, so error-dense corpora still get full coverage.
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "lendiza_export.h"

namespace {

using Disasm = size_t (*)(const uint8_t*, size_t);

Disasm pick_disasm(const std::string& arch)
{
    if (arch == "x86") {
        return &lendiza_disasm_x86;
    }
    if (arch != "x64") {
        std::cerr << "unknown --arch " << arch << "\n";
        std::exit(2);
    }
    return &lendiza_disasm_x64;
}

std::vector<uint8_t> load_file(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "cannot open " << path << "\n";
        std::exit(2);
    }
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>{}};
}

/* Deterministic synthetic corpus: xorshift64* noise tiled to 8 MiB.  Not a
 * realistic instruction distribution (errors dominate), but it exercises
 * every opcode slot and is identical on every machine. */
std::vector<uint8_t> synthetic_corpus()
{
    constexpr std::size_t kSize = 8u * 1024u * 1024u;
    std::vector<uint8_t> buf(kSize);
    std::uint64_t s = 0x9E3779B97F4A7C15ull;
    for (std::size_t i = 0; i < kSize; ++i) {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        buf[i] = static_cast<uint8_t>((s * 0x2545F4914F6CDD1Dull) >> 56);
    }
    return buf;
}

struct SweepResult {
    std::uint64_t decoded = 0;
    std::uint64_t errors = 0;
    std::uint64_t sum_len = 0;
    std::uint64_t checksum = 1469598103934665603ull; /* FNV-1a */
};

SweepResult sweep(const std::vector<uint8_t>& buf, Disasm disasm)
{
    SweepResult r;
    std::size_t off = 0;
    const std::size_t n = buf.size();
    while (off < n) {
        const std::size_t window = std::min<std::size_t>(LENDIZA_MAX_INSNS_LEN, n - off);
        const size_t res = disasm(buf.data() + off, window);
        if (res >= LENDIZA_ERR_INSUFFICIENT_BUFFER) {
            ++r.errors;
            r.checksum = (r.checksum ^ 0xF0u) * 1099511628211ull;
            ++off;
        } else {
            ++r.decoded;
            r.sum_len += res;
            r.checksum = (r.checksum ^ static_cast<std::uint8_t>(res)) * 1099511628211ull;
            off += res;
        }
    }
    return r;
}

void dump(const std::vector<uint8_t>& buf, Disasm disasm, const std::string& out_path)
{
    std::vector<uint8_t> out(buf.size());
    std::size_t off = 0;
    const std::size_t n = buf.size();
    while (off < n) {
        const std::size_t window = std::min<std::size_t>(LENDIZA_MAX_INSNS_LEN, n - off);
        const size_t res = disasm(buf.data() + off, window);
        if (res >= LENDIZA_ERR_INSUFFICIENT_BUFFER) {
            out[off] = static_cast<uint8_t>(0xF0u | (res & 0x0Fu));
            ++off;
        } else {
            out[off] = static_cast<uint8_t>(res);
            off += res;
        }
    }
    std::ofstream f(out_path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
}

double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

} // namespace

int main(int argc, char** argv)
{
    std::vector<std::string> files;
    std::string arch = "x64";
    std::string dump_prefix;
    int iters = 20;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--arch" && i + 1 < argc) {
            arch = argv[++i];
        } else if (a == "--iters" && i + 1 < argc) {
            iters = std::atoi(argv[++i]);
        } else if (a == "--dump" && i + 1 < argc) {
            dump_prefix = argv[++i];
        } else {
            files.emplace_back(a);
        }
    }

    Disasm disasm = pick_disasm(arch);

    std::vector<std::pair<std::string, std::vector<uint8_t>>> corpora;
    if (files.empty()) {
        corpora.emplace_back("synthetic-8MiB", synthetic_corpus());
    } else {
        for (const auto& f : files) {
            corpora.emplace_back(f, load_file(f));
        }
    }

    if (!dump_prefix.empty()) {
        /* One dump per corpus; the caller concatenates or compares per file. */
        for (std::size_t i = 0; i < corpora.size(); ++i) {
            std::string out = dump_prefix;
            if (corpora.size() > 1) {
                out += "." + std::to_string(i);
            }
            dump(corpora[i].second, disasm, out);
            SweepResult r = sweep(corpora[i].second, disasm);
            std::cout << "dumped " << out << " decoded=" << r.decoded << " errors=" << r.errors
                      << " fnv=" << r.checksum << "\n";
        }
        return 0;
    }

    std::cout << "arch=" << arch << " iters=" << iters << "\n";
    for (const auto& [name, buf] : corpora) {
        std::vector<double> samples;
        samples.reserve(iters);
        SweepResult last;
        for (int it = 0; it < iters; ++it) {
            const auto t0 = std::chrono::steady_clock::now();
            last = sweep(buf, disasm);
            const auto t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration<double>(t1 - t0).count());
        }
        const double best = *std::min_element(samples.begin(), samples.end());
        const double med = median(samples);
        const double mib = static_cast<double>(buf.size()) / (1024.0 * 1024.0);
        std::cout << name << " (" << buf.size() << " B)\n"
                  << "  decoded=" << last.decoded << " errors=" << last.errors
                  << " sum_len=" << last.sum_len << " fnv=" << last.checksum << "\n"
                  << std::fixed
                  << "  best  " << mib / best << " MiB/s (" << last.decoded / best / 1e6
                  << " Minstr/s)\n"
                  << "  med   " << mib / med << " MiB/s (" << last.decoded / med / 1e6
                  << " Minstr/s)\n";
    }
    return 0;
}
