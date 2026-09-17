#ifndef LENDIZA_EXPORT_H
#define LENDIZA_EXPORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Longest possible x86 instruction, including prefixes.  Hand the decoder
 * min(LENDIZA_MAX_INSNS_LEN, readable bytes) from a buffer you own. */
#define LENDIZA_MAX_INSNS_LEN 15u

/* C-friendly error codes aligned with lendiza::ErrorCode */
#define LENDIZA_ERR_INSUFFICIENT_BUFFER ((size_t)0xE0)
#define LENDIZA_ERR_UNDEFINED_INSTRUCTION ((size_t)0xE1)
#define LENDIZA_ERR_UNKNOWN ((size_t)0xE2)

/* Length-disassemble a single instruction in 32-bit mode.
 *
 * The result is either the length of the whole instruction - prefix bytes
 * included, since a prefix belongs to the instruction it precedes - or one of
 * the error codes above.  A buffer too short to hold the complete encoding
 * reports LENDIZA_ERR_INSUFFICIENT_BUFFER; bytes beyond buffer[length-1] are
 * never read.
 *
 * VEX (0xC4/0xC5) and EVEX (0x62) encodings are not decoded and report
 * LENDIZA_ERR_UNDEFINED_INSTRUCTION.  So does an encoding whose ModRM names a
 * register where the instruction requires memory (0x8D LEA with mod=11), which
 * the CPU cannot execute at all; that judgement comes from the bytes, not from
 * what the referenced registers happen to point at.
 */
size_t lendiza_disasm_x86(const uint8_t *buffer, size_t length);

/* Length-disassemble a single instruction in 64-bit mode.  Same contract as
 * lendiza_disasm_x86, with REX prefixes accepted. */
size_t lendiza_disasm_x64(const uint8_t *buffer, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* LENDIZA_EXPORT_H */
