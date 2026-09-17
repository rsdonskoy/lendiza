/* GAS wrapper around lde64's LDE, needed for two upstream defects in
 * Includes/opcodes.asm.
 *
 * 1. The prologue in LDE64.asm saves rbp/rcx/rdx/rsi but not rbx, while
 *    PrefAdSize writes ebx -- so every decode of a 0x67-prefixed instruction
 *    destroys a register the Microsoft x64 ABI says is callee-saved. Callers
 *    survive it only until their own epilogue reloads something through the
 *    trashed pointer, which is why the failure surfaces long after the call.
 * 2. PrefAdSize then reads that same ebx as its address-size value:
 *
 *        mov ecx, d [AddressSize]
 *        shr ecx, 1
 *        mov d [AddressSize], ebx      <- PrefOpSize's sibling uses ecx here
 *
 *    so the decoded length of a 0x67 instruction depends on whatever the
 *    caller left in rbx.
 *
 * Loading rbx with the value the handler was meant to use makes single-0x67
 * encodings decode as documented (64-bit mode: 67 selects 32-bit addressing,
 * 32-bit mode: it selects 16), and the push/pop returns rbx to the caller.
 * Two or more chained 0x67 prefixes still mis-size, which no caller-side fix
 * can reach.
 */
    .text
    .globl  lde64_decode

/* size_t lde64_decode(const void* insn, unsigned int arch)
 *   arch: 0 == IA-32, 64 == EM64T  (lde64's second argument is a mode
 *   selector, not a buffer bound; it never receives one). */
lde64_decode:
    pushq   %rbx
    subq    $32, %rsp                 /* shadow space; keeps rsp 16-byte aligned */
    movl    $16, %ebx
    cmpl    $64, %edx
    jne     1f
    movl    $32, %ebx                 /* 67 in 64-bit mode -> 32-bit addressing */
1:  call    LDE                       /* rcx = insn, edx = arch, unchanged */
    addq    $32, %rsp
    popq    %rbx
    ret
