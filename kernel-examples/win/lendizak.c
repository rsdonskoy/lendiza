/* lendizak.c - minimal Windows kernel driver that decodes x86 code with lendiza.

   Its only job is to show the two things a driver must get right:
     1. decoding kernel-resident bytes (here, a static array and its own code), and
     2. decoding user-mode bytes only after they are probed and copied into a
        kernel window - lendiza reads what you hand it and never probes itself.

   Build with the WDK environment (see SOURCES and README.md here), or run
   scripts\check_kernel_compile.cmd for a compile-only verification. */

#include <ntddk.h>

#include "lendiza_ksample.h"

#ifndef LENDIZA_WIN64
#define LENDIZA_WIN64 TRUE
#endif

/* Sample bytes: push / REX.W mov / movabs r64,imm64 / mov ax,imm16 /
 * mov rax, gs:[0] / ret */
static UCHAR g_Code[] = {
    0x55,
    0x48, 0x89, 0xE5,
    0x48, 0xB8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x66, 0xB8, 0x34, 0x12,
    0x64, 0x48, 0x8B, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00,
    0xC3
};

/* A user-mode caller hands over an address; the driver must never decode it in
 * place at IRQL <= APC_LEVEL without probing and copying first. */
static NTSTATUS
LendizaDecodeUserBuffer(
    _In_ PUCHAR UserCode,
    _Out_ PSIZE_T Length_out
    )
{
    UCHAR window[LDZ_MAX_INSNS_LEN];
    MM_COPY_ADDRESS source;
    SIZE_T copied = 0;
    NTSTATUS status;

    if (UserCode == NULL || Length_out == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    __try {
        ProbeForRead(UserCode, LDZ_MAX_INSNS_LEN, sizeof(UCHAR));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    source.VirtualAddress = UserCode;
    status = MmCopyMemory(window, source, LDZ_MAX_INSNS_LEN, MM_COPY_MEMORY_VIRTUAL, &copied);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    if (copied == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    return LendizaKernelDecode(window, copied, LENDIZA_WIN64, Length_out);
}

static void
LendizaDecodeStatic(
    void
    )
{
    SIZE_T instructions = 0;
    const SIZE_T offset = LendizaKernelScan(g_Code, sizeof(g_Code), LENDIZA_WIN64, &instructions);

    KdPrint(("lendizak: static buffer -> %u instructions, %u of %u bytes\n",
             (unsigned)instructions, (unsigned)offset, (unsigned)sizeof(g_Code)));
}

static void
LendizaUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    KdPrint(("lendizak: unload\n"));
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    SIZE_T len = 0;

    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = LendizaUnload;
    LendizaDecodeStatic();

    /* Decoding the driver's own entry point shows a real executable page can be
     * walked from running code, not only from a static buffer. */
    if (NT_SUCCESS(LendizaKernelDecode((PUCHAR)DriverEntry, LDZ_MAX_INSNS_LEN, LENDIZA_WIN64,
                                       &len))) {
        KdPrint(("lendizak: DriverEntry starts with a %u-byte instruction\n", (unsigned)len));
    } else {
        KdPrint(("lendizak: could not decode DriverEntry\n"));
    }

    (void)LendizaDecodeUserBuffer; /* a real driver calls this from an ioctl path */
    return STATUS_SUCCESS;
}
