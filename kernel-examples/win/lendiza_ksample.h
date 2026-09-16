/* lendiza_ksample.h - prototypes shared by the C++ decoder shim and the C driver. */
#ifndef LENDIZA_KSAMPLE_H
#define LENDIZA_KSAMPLE_H

#include <ntddk.h>

#include "lendiza_kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Decodes one instruction from a kernel-accessible buffer.
 * Returns STATUS_SUCCESS with *Length_out set to the instruction length,
 * STATUS_BUFFER_TOO_SMALL when the window cannot hold the whole encoding, or
 * STATUS_INVALID_IMAGE_FORMAT when the bytes are not a decodable instruction. */
NTSTATUS
LendizaKernelDecode(
    _In_reads_bytes_(Length) const PUCHAR Buffer,
    _In_ SIZE_T Length,
    _In_ BOOLEAN LongMode,
    _Out_ PSIZE_T Length_out
    );

/* Walks a code region until decoding stops; returns bytes consumed and stores the
 * number of decoded instructions. */
SIZE_T
LendizaKernelScan(
    _In_reads_bytes_(Length) const PUCHAR Buffer,
    _In_ SIZE_T Length,
    _In_ BOOLEAN LongMode,
    _Out_ PSIZE_T Instructions
    );

#ifdef __cplusplus
}
#endif

#endif /* LENDIZA_KSAMPLE_H */
