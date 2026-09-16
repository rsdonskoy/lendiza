// lendiza_corpus.h - deterministic input corpus shared by the oracle baseline
// dumper and the regression test. No RNG: a fixed-seed LCG keeps every run and
// every compiler in agreement.
#ifndef LENDIZA_CORPUS_H
#define LENDIZA_CORPUS_H

#include <cstddef>
#include <cstdint>

#include "lendiza_code_corpus.h"

namespace lzc {

struct Case {
    uint8_t bytes[16];
    uint8_t len;
};

// Number of distinct opcode/prefix probes walked at full instruction length.
constexpr int kTailProbeCount = 12;
constexpr int kPrefixProbeCount = 12;

inline uint32_t lcg(uint32_t& s)
{
    s = s * 1664525u + 1013904223u;
    return s >> 8;
}

// Prefix alphabet: legacy overrides, REX encodings and the 0F escape.
inline const uint8_t* prefix_alphabet(int& count)
{
    static const uint8_t p[] = {
        0x66, 0x67, 0x2E, 0x36, 0x64, 0x65, 0xF0, 0xF2, 0xF3, 0x40, 0x41, 0x48, 0x4B, 0x0F
    };
    count = static_cast<int>(sizeof(p) / sizeof(p[0]));
    return p;
}

// ModRM/SIB/imm bytes that move the length model: mod/rm boundaries, SIB base=5,
// disp/imm bytes that can be mistaken for more prefixes.
inline const uint8_t* tail_alphabet(int& count)
{
    static const uint8_t t[] = {
        0x00, 0x05, 0x0D, 0x25, 0x35, 0x45, 0x8D, 0xA5, 0xC5, 0xE5, 0xFF, 0x80
    };
    count = static_cast<int>(sizeof(t) / sizeof(t[0]));
    return t;
}

class Corpus {
public:
    explicit Corpus(void (*sink)(const Case&, void*), void* ctx) : sink_(sink), ctx_(ctx) {}

    void run()
    {
        emit_exhaustive(1);
        emit_exhaustive(2);
        emit_restricted(3);
        emit_prefix_walks();
        emit_modrm_matrix();
        emit_shellcode_windows();
    }

    size_t count() const { return count_; }

private:
    void emit(const uint8_t* p, size_t n)
    {
        Case c{};
        if (n > sizeof(c.bytes)) {
            n = sizeof(c.bytes);
        }
        for (size_t i = 0; i < n; ++i) {
            c.bytes[i] = p[i];
        }
        c.len = static_cast<uint8_t>(n);
        sink_(c, ctx_);
        ++count_;
    }

    // Every byte (n=1) and every 16-bit pattern (n=2).
    void emit_exhaustive(int n)
    {
        if (n == 1) {
            for (uint32_t a = 0; a < 256; ++a) {
                const uint8_t b[1] = { static_cast<uint8_t>(a) };
                emit(b, 1);
            }
            return;
        }
        for (uint32_t v = 0; v < 65536; ++v) {
            const uint8_t b[2] = { static_cast<uint8_t>(v >> 8), static_cast<uint8_t>(v & 0xFF) };
            emit(b, 2);
        }
    }

    // Exhaustive 3-byte sequences over the prefix/ModRM-significant alphabet.
    void emit_restricted(int n)
    {
        int pc = 0;
        const uint8_t* pfx = prefix_alphabet(pc);
        int tc = 0;
        const uint8_t* tail = tail_alphabet(tc);
        if (n != 3) {
            return;
        }
        uint8_t b[3];
        for (int i = 0; i < pc; ++i) {
            for (int j = 0; j < pc; ++j) {
                for (int k = 0; k < tc; ++k) {
                    b[0] = pfx[i];
                    b[1] = pfx[j];
                    b[2] = tail[k];
                    emit(b, 3);
                }
            }
        }
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < tc; ++j) {
                for (int k = 0; k < tc; ++k) {
                    b[0] = static_cast<uint8_t>(i);
                    b[1] = tail[j];
                    b[2] = tail[k];
                    emit(b, 3);
                }
            }
        }
    }

    // Full-length walks: [prefix(es)] [opcode] [tail...] up to 15 bytes, so the
    // decoder sees complete encodings rather than only truncated ones.
    void emit_prefix_walks()
    {
        int pc = 0;
        const uint8_t* pfx = prefix_alphabet(pc);
        int tc = 0;
        const uint8_t* tail = tail_alphabet(tc);
        uint8_t b[15];
        for (int oi = 0; oi < 256; ++oi) {
            for (int pi = 0; pi < pc; ++pi) {
                for (int tj = 0; tj < tc; ++tj) {
                    for (int n = 3; n <= 10; ++n) {
                        b[0] = pfx[pi];
                        b[1] = static_cast<uint8_t>(oi);
                        b[2] = tail[tj];
                        for (int k = 3; k < n; ++k) {
                            b[k] = tail[(tj + k) % tc];
                        }
                        emit(b, static_cast<size_t>(n));
                    }
                }
            }
        }
        // Two prefixes (e.g. 67 66, 64 67, REX + 66) in front of every opcode.
        static const int kTwoPrefixLens[4] = { 4, 7, 10, 15 };
        for (int oi = 0; oi < 256; ++oi) {
            for (int i = 0; i < pc; ++i) {
                for (int j = 0; j < pc; ++j) {
                    for (int l = 0; l < 4; ++l) {
                        const int n = kTwoPrefixLens[l];
                        b[0] = pfx[i];
                        b[1] = pfx[j];
                        b[2] = static_cast<uint8_t>(oi);
                        for (int k = 3; k < n; ++k) {
                            b[k] = static_cast<uint8_t>(0x40 + k);
                        }
                        emit(b, static_cast<size_t>(n));
                    }
                }
            }
        }
    }

    // Every ModRM byte with and without REX.B, in both operand/address sizes.
    void emit_modrm_matrix()
    {
        static const uint8_t base_opc[] = { 0x00, 0x03, 0x8D, 0x8B, 0xC7, 0xF7, 0xFF, 0x83, 0x0F };
        uint8_t b[15];
        for (size_t o = 0; o < sizeof(base_opc); ++o) {
            for (int m = 0; m < 256; ++m) {
                for (int p = 0; p < 4; ++p) {
                    static const uint8_t pre[4][2] = {
                        { 0x00, 0x00 }, { 0x48, 0x00 }, { 0x41, 0x00 }, { 0x67, 0x66 }
                    };
                    const int pn = (pre[p][1] == 0) ? 1 : 2;
                    if (pn == 1 && pre[p][0] == 0x00) {
                        continue;
                    }
                    b[0] = pre[p][0];
                    b[1] = pre[p][1];
                    int idx = pn;
                    b[idx++] = base_opc[o];
                    if (base_opc[o] == 0x0F) {
                        b[idx++] = 0xA2;
                    }
                    b[idx++] = static_cast<uint8_t>(m);
                    for (int k = idx; k < 15; ++k) {
                        b[k] = static_cast<uint8_t>(0x25 + (k % 3));
                    }
                    emit(b, static_cast<size_t>(idx + 4));
                }
            }
        }
    }

    // Sliding windows of real code bytes, truncated at every length 1..15.
    void emit_shellcode_windows()
    {
        size_t n = 0;
        const uint8_t* code = lzc::shellcode(n);
        for (size_t off = 0; off < n; ++off) {
            for (size_t w = 1; w <= 15 && off + w <= n; ++w) {
                emit(code + off, w);
            }
        }

        // Linear scan: feed a 15-byte window, then step by the consumed length, so
        // the sequence itself (not just isolated buffers) is part of the baseline.
        for (size_t off = 0; off < n;) {
            size_t w = 15;
            if (off + w > n) {
                w = n - off;
            }
            emit(code + off, w);
            off += w;
        }
    }

    void (*sink_)(const Case&, void*);
    void* ctx_;
    size_t count_ { 0 };
};

} // namespace lzc

#endif // LENDIZA_CORPUS_H
