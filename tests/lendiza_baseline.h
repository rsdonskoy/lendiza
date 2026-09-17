// lendiza_baseline.h - shared replay of the committed behaviour baseline.
//
// The corpus in lendiza_corpus.h is replayed against the decoder and compared
// with tests/data/oracle_before.bin (recorded from the pre-refactor decoder).
// Every difference must fall into one of the categories below; anything else is
// a regression and makes the comparison fail.
#ifndef LENDIZA_BASELINE_H
#define LENDIZA_BASELINE_H

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "lendiza_corpus.h"

namespace lzbase {

struct Header {
    char magic[4];
    uint32_t version;
    uint32_t count;
};

enum Category {
    cat_same = 0,
    cat_prefix,        /* a prefix precedes the opcode and is now counted in */
    cat_truncated,     /* the baseline answered with bytes it did not have */
    cat_group2_count,  /* 0F 00 / 0F 01 memory forms now count ModRM + disp */
    cat_group_disp,    /* F6/F7/FE/FF/C6/C7 now count SIB and displacement */
    cat_error_refined, /* 0F family: error code refined, or a length where the
                        * baseline only knew "invalid" */
    cat_ia32_row_a,    /* IA-32 two-byte map row A (SHLD/SHRD imm8, PUSH/POP GS,
                        * RSM, BTS, IMUL): every cell there said "six bytes" */
    cat_reg_operand_invalid, /* the baseline gave a length to an encoding whose
                              * ModRM names a register where the instruction
                              * requires memory, which the CPU cannot execute */
    cat_unexpected,
    cat_count          /* number of categories; the bound for Result::counts */
};

inline const char* category_name(Category c)
{
    switch (c) {
    case cat_prefix: return "prefix";
    case cat_truncated: return "truncated";
    case cat_group2_count: return "group2-count";
    case cat_group_disp: return "group-disp";
    case cat_error_refined: return "error-refined";
    case cat_ia32_row_a: return "ia32-row-a";
    case cat_reg_operand_invalid: return "reg-operand-invalid";
    case cat_unexpected: return "UNEXPECTED";
    default: return "same";
    }
}

inline bool load(const char* path, std::vector<uint8_t>& out32, std::vector<uint8_t>& out64)
{
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        std::fprintf(stderr, "cannot open baseline %s (set LENDIZA_ORACLE_PATH)\n", path);
        return false;
    }
    Header h {};
    if (std::fread(&h, sizeof(h), 1, f) != 1 || std::memcmp(h.magic, "LDZB", 4) != 0) {
        std::fprintf(stderr, "bad baseline file %s\n", path);
        std::fclose(f);
        return false;
    }
    out32.resize(h.count);
    out64.resize(h.count);
    const bool ok = std::fread(out32.data(), 1, h.count, f) == h.count &&
                    std::fread(out64.data(), 1, h.count, f) == h.count;
    std::fclose(f);
    return ok;
}

inline bool is_prefix_byte(uint8_t b, bool long_mode)
{
    switch (b) {
    case 0x26: case 0x2E: case 0x36: case 0x3E: case 0x64: case 0x65:
    case 0x66: case 0x67: case 0xF0: case 0xF2: case 0xF3:
        return true;
    default:
        break;
    }
    /* REX only exists in long mode; VEX/EVEX are rejected there. */
    return long_mode && ((b >= 0x40 && b <= 0x4F) || b == 0xC4 || b == 0xC5 || b == 0x62);
}

/* Maps one difference onto the reviewed reason that allows it. */
inline Category classify(const lzc::Case& c, uint8_t before, uint8_t after, bool long_mode)
{
    if (before == after) {
        return cat_same;
    }
    if (is_prefix_byte(c.bytes[0], long_mode)) {
        return cat_prefix;
    }
    if (!long_mode && c.bytes[0] == 0x0F && c.len > 1) {
        switch (c.bytes[1]) {
        case 0xA4: case 0xA8: case 0xA9: case 0xAA:
        case 0xAB: case 0xAC: case 0xAD: case 0xAF:
            return cat_ia32_row_a; /* the six-byte cells of the IA-32 row A fix */
        default:
            break;
        }
    }
    if (c.bytes[0] == 0x8D && c.len >= 2 && (c.bytes[1] & 0xC0u) == 0xC0u &&
        after == 0xE1) {
        return cat_reg_operand_invalid; /* LEA naming a register: not executable */
    }
    if (before >= 0xE0) {
        /* Reviewed: within the 0F escape family the baseline either demanded a
         * byte it did not need or called a lengthable encoding invalid.  Any
         * other baseline error must stay the same error. */
        return c.bytes[0] == 0x0F ? cat_error_refined : cat_unexpected;
    }
    if (static_cast<size_t>(before) > c.len ||
        (after >= 0xE0 && static_cast<size_t>(before + 1) > c.len)) {
        return cat_truncated;
    }
    if (c.bytes[0] == 0x0F && c.len > 1 && (c.bytes[1] == 0x00 || c.bytes[1] == 0x01)) {
        return cat_group2_count;
    }
    switch (c.bytes[0]) {
    case 0xF6: case 0xF7: case 0xFE: case 0xFF: case 0xC6: case 0xC7:
        return cat_group_disp;
    default:
        break;
    }
    (void)after;
    return cat_unexpected;
}

struct Result {
    size_t differences = 0;
    /* indexed by Category; cat_count is the bound - a hardcoded size here once
     * let `++r.counts[cat_unexpected]` write past the array into the vector
     * below. */
    size_t counts[cat_count] = {};
    std::vector<std::string> unexpected_samples;
};

/* Rebuilds the corpus in emission order so a result index maps back to bytes. */
inline void collect(const lzc::Case& c, void* ctx)
{
    static_cast<std::vector<lzc::Case>*>(ctx)->push_back(c);
}

inline void accumulate(Result& r, const std::vector<lzc::Case>& cases,
                       const std::vector<uint8_t>& before, const std::vector<uint8_t>& after,
                       bool long_mode, size_t max_samples)
{
    for (size_t i = 0; i < before.size() && i < cases.size(); ++i) {
        const Category cat = classify(cases[i], before[i], after[i], long_mode);
        ++r.counts[cat];
        if (cat == cat_same) {
            continue;
        }
        ++r.differences;
        if (cat == cat_unexpected && r.unexpected_samples.size() < max_samples) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "bytes=");
            for (size_t k = 0; k < cases[i].len; ++k) {
                std::snprintf(buf + std::strlen(buf), sizeof(buf) - std::strlen(buf), "%02X",
                              cases[i].bytes[k]);
            }
            std::snprintf(buf + std::strlen(buf), sizeof(buf) - std::strlen(buf), " before=%u after=%u",
                          before[i], after[i]);
            r.unexpected_samples.push_back(buf);
        }
    }
}

} // namespace lzbase

#endif // LENDIZA_BASELINE_H
