// lendiza_decode.cpp - how a Windows kernel driver consumes lendiza.
//
// Built with: cl /kernel /std:c++17 /GR- /EHs-c- /I<wdk>\km /I<wdk>\km\crt /I<wdk>\shared
//             /I<lendiza>\include /c lendiza_decode.cpp
//
// Only lendiza_core.hpp is included: no CRT, no STL, no exceptions, no RTTI, no
// virtual dispatch and no mutable global state, so this function is safe to call
// at DISPATCH_LEVEL or from an ISR.  Compile /W4 /WX and check the object with
// dumpbin /symbols to confirm nothing outside ntoskrnl is referenced.
//
// The caller owns the memory: pass kernel-accessible bytes.  When decoding code
// that lives in a user-mode buffer, copy it in first (ProbeForRead +
// ExAllocatePoolUninitialized, or MmCopyMemory) - the decoder never probes.

#include <ntddk.h>

#include "lendiza_core.hpp"
#include "lendiza_ksample.h"

// Decodes one instruction and reports its length, or the reason it failed.
// The buffer must hold a complete encoding; size a code window with
// LDZ_MAX_INSNS_LEN (15).
NTSTATUS
LendizaKernelDecode(
    _In_reads_bytes_(Length) const PUCHAR Buffer,
    _In_ SIZE_T Length,
    _In_ BOOLEAN LongMode,
    _Out_ PSIZE_T Length_out
    )
{
    if (Buffer == nullptr || Length_out == nullptr || Length == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    const ldz_u8* bytes = static_cast<const ldz_u8*>(Buffer);
    const size_t len = LongMode ? lendiza::disasm_x64(bytes, Length)
                                : lendiza::disasm_x86(bytes, Length);

    if (lendiza::is_error(len)) {
        // 0xE0: the window was too small to hold a complete instruction.
        // 0xE1: undefined encoding (including VEX/EVEX, which is not decoded).
        return len == LDZ_ERR_INSUFFICIENT_BUFFER ? STATUS_BUFFER_TOO_SMALL
                                                  : STATUS_INVALID_IMAGE_FORMAT;
    }
    if (len == 0) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    *Length_out = len;
    return STATUS_SUCCESS;
}

// Walks a code region as far as it decodes cleanly.  Returns the number of bytes
// consumed and the number of instructions decoded.
SIZE_T
LendizaKernelScan(
    _In_reads_bytes_(Length) const PUCHAR Buffer,
    _In_ SIZE_T Length,
    _In_ BOOLEAN LongMode,
    _Out_ PSIZE_T Instructions
    )
{
    SIZE_T offset = 0;
    SIZE_T count = 0;

    if (Buffer == nullptr || Instructions == nullptr) {
        return 0;
    }

    while (offset < Length) {
        // Hand the decoder a bounded window so a truncated tail is reported as
        // "not enough bytes" instead of reading further into the region.
        const SIZE_T remaining = Length - offset;
        const SIZE_T window = remaining < LDZ_MAX_INSNS_LEN ? remaining : LDZ_MAX_INSNS_LEN;

        SIZE_T insn = 0;
        if (!NT_SUCCESS(LendizaKernelDecode(Buffer + offset, window, LongMode, &insn))) {
            break;
        }
        offset += insn;
        ++count;
    }

    *Instructions = count;
    return offset;
}
