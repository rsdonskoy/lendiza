// SPDX-License-Identifier: MIT
/* lendiza_kmod.c - Linux kernel module that decodes x86 code with lendiza.
 *
 * The module itself is C; the decode core is C++ (lendiza_kshim.cpp) built by the
 * custom rule in Makefile and linked like any other object.  Two things are
 * shown: decoding kernel-resident code (this module's own text), and decoding
 * user memory only after copy_from_user - lendiza never probes an address itself.
 *
 * Build:  make -C /lib/modules/$(uname -r)/build M=$PWD modules
 * Load:   sudo insmod lendiza_demo.ko ; dmesg | tail
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>

#define LENDIZA_MAX_INSNS_LEN 15

/* Implemented in lendiza_kshim.cpp.  Sizes match the kernel's size_t/uint8_t. */
long lendiza_kshim_decode(const u8* bytes, size_t n, int long_mode);
size_t lendiza_kshim_walk(const u8* bytes, size_t n, int long_mode, size_t* count);

/* Maps a raw decoder result onto errno. */
static int lendiza_errno(long r)
{
    if (r >= 0 && r < 0xE0) {
        return 0;
    }
    return r == 0xE0 ? -ENOSPC : -EILSEQ;
}

static bool long_mode = true;
module_param(long_mode, bool, 0444);
MODULE_PARM_DESC(long_mode, "decode 64-bit code (default) or IA-32 code");

static char* decode_addr;
module_param(decode_addr, charp, 0444);
MODULE_PARM_DESC(decode_addr, "hex address of user code to decode at load, e.g. 0x7ffd1234");

/* push / REX.W mov / movabs r64,imm64 / mov ax,imm16 / mov rax,gs:[0] / ret */
static const u8 sample_code[] = {
    0x55,
    0x48, 0x89, 0xe5,
    0x48, 0xb8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x66, 0xb8, 0x34, 0x12,
    0x65, 0x48, 0x8b, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00,
    0xc3
};

static int __init lendiza_init(void)
{
    size_t instructions = 0;
    size_t consumed;
    long first;

    consumed = lendiza_kshim_walk(sample_code, sizeof(sample_code), long_mode, &instructions);
    pr_info("lendiza: sample buffer -> %zu instructions, %zu of %zu bytes\n", instructions,
            consumed, sizeof(sample_code));
    if (consumed != sizeof(sample_code)) {
        pr_warn("lendiza: sample walk stopped early at offset %zu\n", consumed);
    }

    /* A 2-byte window cannot hold movabs rax, imm64: the core must say so rather
     * than return a length it could not have read. */
    first = lendiza_kshim_decode(sample_code, 2, long_mode);
    pr_info("lendiza: 2-byte window -> %ld (%d)\n", first, lendiza_errno(first));

    /* Decoding the module's own text proves a real executable page can be walked. */
    consumed = lendiza_kshim_walk((const u8*)lendiza_init, LENDIZA_MAX_INSNS_LEN, long_mode,
                                  &instructions);
    pr_info("lendiza: this function's first %zu byte(s) decoded as %zu instructions\n", consumed,
            instructions);

    if (decode_addr) {
        u8 window[LENDIZA_MAX_INSNS_LEN];
        const void __user *user_code;
        unsigned long not_copied;
        u64 addr = 0;
        long r;

        if (kstrtou64(decode_addr, 0, &addr)) {
            pr_warn("lendiza: cannot parse decode_addr=%s\n", decode_addr);
        } else {
            user_code = (const void __user *)(unsigned long)addr;
            not_copied = copy_from_user(window, user_code, sizeof(window));
            if (not_copied == sizeof(window)) {
                pr_warn("lendiza: %s is not readable\n", decode_addr);
            } else {
                r = lendiza_kshim_decode(window, sizeof(window) - not_copied, long_mode);
                pr_info("lendiza: %s -> %ld bytes (errno %d)\n", decode_addr, r, lendiza_errno(r));
            }
        }
    }

    return 0;
}

static void __exit lendiza_exit(void)
{
    pr_info("lendiza: unloaded\n");
}

module_init(lendiza_init);
module_exit(lendiza_exit);

MODULE_LICENSE("MIT");
MODULE_DESCRIPTION("Length disassembly of x86 code in kernel mode, via lendiza");
MODULE_AUTHOR("lendiza contributors");
