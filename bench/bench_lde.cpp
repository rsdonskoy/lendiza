/* Linear-sweep comparison of lendiza against lde64 over the same buffer.
 * Both return an instruction length and nothing else, so elapsed time and
 * successfully decoded instruction counts are the whole measurement.
 *
 * lde64 takes an architecture selector as its second argument (0 == IA-32,
 * 64 == EM64T) rather than a length, so anything other than 64 here decodes
 * 64-bit code as IA-32 and every REX byte comes back as a one-byte inc/dec.
 * It is never given a bound either, so it can read past whatever lendiza's
 * clamped window allows; the corpus therefore gets trailing slack bytes.
 *
 * Calls go through lde64_decode in lde64_stub.s, which repairs the RBX that
 * upstream's 0x67 handler destroys.
 */

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "lendiza_core.hpp"
#include "lendiza_export.h"

/* Declared here rather than via lde64.h, which omits extern "C" and so mangles
 * the reference against a C-symbol archive. The wrapper lives in lde64_stub.s. */
extern "C" auto lde64_decode(const void* lpData, unsigned int arch) -> std::size_t;

namespace {

/* lde64's architecture selector: 0 selects IA-32 decoding. */
constexpr unsigned kLdeEm64t = 64;

constexpr std::size_t kSlack = 64;

struct Options {
    std::vector<std::string> fixtures;
    std::string file_path;
    std::uint32_t iters = 40;
};

struct Stats {
    std::string name;
    double seconds = 0.0;
    std::uint64_t bytes = 0;
    std::uint64_t decoded = 0;
    std::uint64_t errors = 0;
    std::uint64_t sum_len = 0;
};

auto LoadFile(const std::string& path) -> std::vector<std::uint8_t>
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                     std::istreambuf_iterator<char>{});
}

auto FixturePath(const std::string& name) -> std::string
{
    const auto dir = std::filesystem::path(LENDIZA_BENCH_FIXTURE_DIR);
    return (dir / (name + ".text.bin")).string();
}

auto ParseOptions(const int argc, char** argv) -> Options
{
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string arg{argv[i]};
        if (arg == "--fixture" && i + 1 < argc) {
            opt.fixtures.emplace_back(argv[++i]);
        } else if (arg == "--file" && i + 1 < argc) {
            opt.file_path = argv[++i];
        } else if (arg == "--iters" && i + 1 < argc) {
            const auto parsed = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            if (parsed > 0) {
                opt.iters = parsed;
            }
        }
    }
    if (opt.fixtures.empty() && opt.file_path.empty()) {
        opt.fixtures = {"ntdll", "kernelbase", "kernel32"};
    }
    return opt;
}

/* One linear pass over `code`, repeated `iters` times. A decoder that returns
 * an error or a zero length resyncs by one byte, matching how a hooking caller
 * would walk over code it cannot fully decode. */
template <class Decode>
auto Sweep(const std::string& name, const std::vector<std::uint8_t>& code, std::uint32_t iters, Decode decode) -> Stats
{
    Stats s;
    s.name = name;

    const auto t0 = std::chrono::steady_clock::now();
    for (std::uint32_t it = 0; it < iters; ++it) {
        std::size_t off = 0;
        while (off < code.size()) {
            const auto window = std::min<std::size_t>(LDZ_MAX_INSNS_LEN, code.size() - off);
            const auto len = decode(code.data() + off, window);
            s.sum_len += len;
            if (lendiza::is_error(len) || len == 0) {
                ++s.errors;
                ++off;
                continue;
            }
            ++s.decoded;
            off += len;
        }
    }
    const auto t1 = std::chrono::steady_clock::now();

    s.seconds = std::chrono::duration<double>(t1 - t0).count();
    s.bytes = code.size() * static_cast<std::uint64_t>(iters);
    return s;
}

void Print(const Stats& s)
{
    const auto mib = static_cast<double>(s.bytes) / (1024.0 * 1024.0);
    const auto mib_s = s.seconds > 0.0 ? mib / s.seconds : 0.0;
    const auto minsn_s = s.seconds > 0.0 ? static_cast<double>(s.decoded) / s.seconds / 1e6 : 0.0;
    const auto avg_step = s.decoded > 0 ? static_cast<double>(s.sum_len) / static_cast<double>(s.decoded) : 0.0;

    std::cout << std::left << std::setw(20) << s.name
              << std::right << std::fixed << std::setprecision(2)
              << " | " << std::setw(6) << s.seconds << " s"
              << " | " << std::setw(8) << mib_s << " MiB/s"
              << " | " << std::setw(8) << minsn_s << " Minsn/s"
              << " | decoded=" << std::setw(12) << s.decoded
              << " errors=" << std::setw(9) << s.errors
              << " avg_step=" << avg_step << '\n';
}

} // namespace

auto main(const int argc, char** argv) -> int
{
    const auto opt = ParseOptions(argc, argv);

    std::vector<std::uint8_t> raw;
    std::string desc;
    if (!opt.file_path.empty()) {
        raw = LoadFile(opt.file_path);
        desc = opt.file_path;
    } else {
        for (const auto& name : opt.fixtures) {
            const auto part = LoadFile(FixturePath(name));
            if (part.empty()) {
                std::cerr << "error: cannot read fixture " << FixturePath(name) << '\n';
                return 1;
            }
            raw.insert(raw.end(), part.begin(), part.end());
            desc += (desc.empty() ? "" : "+") + name;
        }
    }
    if (raw.empty()) {
        std::cerr << "error: no corpus\n";
        return 1;
    }

    std::vector<std::uint8_t> code(raw.size() + kSlack, 0x90);
    std::memcpy(code.data(), raw.data(), raw.size());

    std::cout << "corpus: " << raw.size() << " bytes (" << desc << "), iters=" << opt.iters << "\n";
    std::cout << "decoder path               | elapsed | throughput |  rate   | counts\n";
    std::cout << "---------------------------------------------------------------------------\n";

    Print(Sweep("lendiza", code, opt.iters,
                [](const std::uint8_t* p, std::size_t n) { return lendiza::disasm_x64(p, n); }));
    Print(Sweep("lendiza (C ABI)", code, opt.iters,
                [](const std::uint8_t* p, std::size_t n) { return lendiza_disasm_x64(p, n); }));
    Print(Sweep("lde64", code, opt.iters,
                [](const std::uint8_t* p, std::size_t) { return lde64_decode(p, kLdeEm64t); }));

    return 0;
}
