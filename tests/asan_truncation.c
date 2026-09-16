/* asan_truncation.c - the same no-over-read claim, checked by AddressSanitizer.
 *
 * Every input is allocated at exactly its own size, so any read past the end is
 * a heap-buffer-overflow that ASan reports with the offending decoder frame.
 * This complements guard_page.cpp: ASan also catches reads that stay inside the
 * same allocation but beyond the requested length, which a page guard cannot see.
 *
 *   cl /std:c++17 /fsanitize=address /Ichc++ /Tas ..\src\lendiza.cpp asan_truncation.c
 *   gcc -std=c11 -fsanitize=address -I../include asan_truncation.c ../src/lendiza.cpp -o asan
 *
 * Built as C on purpose: it proves the extern "C" surface is usable from a C
 * translation unit, which is what a kernel module does.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lendiza_export.h"
#include "lendiza_code_corpus_c.h"

static size_t g_cases = 0;

/* Runs one buffer, allocated to exactly n bytes, through both entry points. */
static void probe(const uint8_t* src, size_t n)
{
    uint8_t* p;
    size_t result;

    if (n == 0 || n > LENDIZA_MAX_INSNS_LEN) {
        return;
    }
    p = (uint8_t*)malloc(n);
    if (p == NULL) {
        perror("malloc");
        exit(2);
    }
    memcpy(p, src, n);

    ++g_cases;
    result = lendiza_disasm_x64(p, n);
    if (result < LENDIZA_ERR_INSUFFICIENT_BUFFER && result > n) {
        printf("  guessed length %zu > n=%zu\n", result, n);
        abort();
    }

    ++g_cases;
    result = lendiza_disasm_x86(p, n);
    if (result < LENDIZA_ERR_INSUFFICIENT_BUFFER && result > n) {
        printf("  guessed length %zu > n=%zu\n", result, n);
        abort();
    }

    free(p);
}

/* Every length from 1 to a complete encoding, so each truncation depth runs. */
static void probe_all_depths(const uint8_t* src, size_t n)
{
    size_t len;
    for (len = 1; len <= n && len <= LENDIZA_MAX_INSNS_LEN; ++len) {
        probe(src, len);
    }
}

int main(void)
{
    size_t i, j, k, off, n_code;
    const uint8_t* code = lendiza_code_corpus;
    n_code = LENDIZA_CODE_CORPUS_LEN;
    (void)lendiza_code_corpus;
    uint8_t buf[LENDIZA_MAX_INSNS_LEN];

    /* 1. Exhaustive one- and two-byte starts over the interesting alphabet. */
    static const uint8_t alpha[] = { 0x00, 0x04, 0x05, 0x0D, 0x0F, 0x24, 0x25, 0x36, 0x3A, 0x40, 0x41,
                                     0x48, 0x4B, 0x62, 0x66, 0x67, 0x69, 0x6B, 0x71, 0x75, 0x80, 0x81,
                                     0x83, 0x84, 0x8D, 0xA1, 0xA4, 0xB8, 0xC4, 0xC5, 0xC6, 0xC7, 0xCC,
                                     0xD0, 0xD8, 0xDF, 0xE8, 0xEA, 0xF2, 0xF3, 0xF6, 0xF7, 0xFA, 0xFF };
    const size_t an = sizeof(alpha) / sizeof(alpha[0]);

    for (i = 0; i < an; ++i) {
        buf[0] = alpha[i];
        probe_all_depths(buf, 1);
        for (j = 0; j < an; ++j) {
            buf[1] = alpha[j];
            probe_all_depths(buf, 2);
            for (k = 0; k < an; ++k) {
                buf[2] = alpha[k];
                probe_all_depths(buf, 3);
            }
        }
    }

    /* 2. Real code at every offset and window. */
    for (off = 0; off < n_code; ++off) {
        size_t avail = n_code - off;
        if (avail > LENDIZA_MAX_INSNS_LEN) {
            avail = LENDIZA_MAX_INSNS_LEN;
        }
        probe_all_depths(code + off, avail);
    }

    /* 3. Every two-byte prefix with a displacement-shaped tail. */
    for (i = 0; i < 256; ++i) {
        for (j = 0; j < 256; ++j) {
            buf[0] = (uint8_t)i;
            buf[1] = (uint8_t)j;
            for (k = 2; k < LENDIZA_MAX_INSNS_LEN; ++k) {
                buf[k] = (k % 2) ? 0x25 : 0x05;
            }
            probe_all_depths(buf, LENDIZA_MAX_INSNS_LEN);
        }
    }

    /* 4. Null and zero-length contracts. */
    if (lendiza_disasm_x64(NULL, 4) != LENDIZA_ERR_INSUFFICIENT_BUFFER) {
        printf("null buffer must report INSUFFICIENT_BUFFER\n");
        return 1;
    }
    if (lendiza_disasm_x86(buf, 0) != LENDIZA_ERR_INSUFFICIENT_BUFFER) {
        printf("zero length must report INSUFFICIENT_BUFFER\n");
        return 1;
    }

    printf("asan truncation matrix: %zu decodes, no out-of-bounds access\n", g_cases);
    return 0;
}
