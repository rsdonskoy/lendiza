// lendiza_group_sweep.h - exhaustive enumeration of the 0xEE/0xEF dispatch
// space (opcode extensions and the x87 family).
//
// tests/tools/dump_group_baseline.cpp records the pre-refactor answers here,
// tests/test_group_oracle.cpp replays them and requires zero differences.
// The enumeration order IS the baseline format: never reorder or insert
// cases while a committed baseline must stay valid.
// This file is locked by tests/data/group_before.bin: any change here needs
// prior human approval plus a stated reason for re-recording the baseline
// (tools/dump_group_baseline.cpp refuses to overwrite that file without
// --force, and the OracleTest suites gate every other difference).
#ifndef LENDIZA_GROUP_SWEEP_H
#define LENDIZA_GROUP_SWEEP_H

#include <cstddef>
#include <cstdint>

namespace lzsw {

using Sink = void (*)(const uint8_t* bytes, uint8_t len, unsigned mode, void* ctx);

inline constexpr uint8_t grp1_opcodes[] { 0xC6, 0xC7, 0xF6, 0xF7, 0xFE, 0xFF };
inline constexpr uint8_t grp2_seconds[] { 0x00, 0x01, 0x18, 0x71, 0x72, 0x73,
                                          0xAE, 0xB9, 0xBA, 0xC7 };
inline constexpr uint8_t x87_opcodes[] { 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF };

/* Five bytes behind the ModRM: an SIB candidate plus disp32 filler.  They
 * only take effect when the decoder reaches them, so mod=11 and rm!=100
 * cases are insensitive to which entry is used, and every mem+rm=100 form
 * is exercised against all four SIB shapes. */
inline constexpr uint8_t fillers[4][5] = {
    { 0x25, 0x11, 0x22, 0x33, 0x44 }, /* base=101, no index -> disp32 */
    { 0x00, 0x11, 0x22, 0x33, 0x44 }, /* eax, no displacement         */
    { 0x8D, 0x11, 0x22, 0x33, 0x44 }, /* mod=01 disp8                 */
    { 0x05, 0x11, 0x22, 0x33, 0x44 }, /* disp32                       */
};

inline void for_each(const Sink sink, void* const ctx)
{
    static const uint8_t pfx32[4][2] = { { 0x00, 0x00 }, { 0x66, 0x00 },
                                         { 0x67, 0x00 }, { 0x66, 0x67 } };
    /* The two 0x41 sets exist because the sweep previously never set REX.B, which
     * is what let a wrong REX.B/SIB rule pass this oracle unnoticed. */
    static const uint8_t pfx64[7][2] = { { 0x00, 0x00 }, { 0x66, 0x00 }, { 0x48, 0x00 },
                                         { 0x66, 0x48 }, { 0x67, 0x48 },
                                         { 0x41, 0x00 }, { 0x66, 0x41 } };

    for (unsigned mode = 32; mode <= 64; mode += 32) {
        const bool m64 = mode == 64;
        const uint8_t(*sets)[2] = m64 ? pfx64 : pfx32;
        const size_t set_count = m64 ? 7 : 4;

        for (size_t ps = 0; ps < set_count; ++ps) {
            uint8_t pfx[2];
            pfx[0] = sets[ps][0];
            pfx[1] = sets[ps][1];

            for (uint8_t family = 0; family < 3; ++family) {
              const uint8_t* list = family == 0 ? grp1_opcodes
                                : family == 1 ? grp2_seconds : x87_opcodes;
              const size_t list_n = family == 1 ? 10 : (family == 0 ? 6 : 8);
              for (size_t oi = 0; oi < list_n; ++oi) {
                const uint8_t opc = list[oi];
                for (int modrm = 0; modrm < 256; ++modrm) {
                  for (uint8_t fi = 0; fi < 4; ++fi) {
                    for (uint8_t mi = 0; mi < 7; ++mi) { /* full, full-1, full-3, 7, 5, 4, 3 */
                      uint8_t buf[16];
                      size_t n = 0;
                      if (pfx[0] != 0) { buf[n++] = pfx[0]; }
                      if (pfx[1] != 0) { buf[n++] = pfx[1]; }
                      if (family == 1) { buf[n++] = 0x0F; }
                      buf[n++] = opc;
                      buf[n++] = modrm;
                      for (uint8_t k = 0; k < 5; ++k) { buf[n++] = fillers[fi][k]; }

                      uint8_t w;
                      if (mi == 0) { w = static_cast<uint8_t>(n); }
                      else if (mi == 1) { w = static_cast<uint8_t>(n - 1); }
                      else if (mi == 2) { w = static_cast<uint8_t>(n - 3); }
                      else if (mi == 3) { w = 7; }
                      else if (mi == 4) { w = 5; }
                      else if (mi == 5) { w = 4; }
                      else { w = 3; }
                      sink(buf, w, mode, ctx);
                    }
                  }
                }
              }
            }
        }
    }
}

} // namespace lzsw

#endif // LENDIZA_GROUP_SWEEP_H
