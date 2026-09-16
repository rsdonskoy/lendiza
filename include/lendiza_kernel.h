/* lendiza_kernel.h - platform type layer for user space and kernel space.
 *
 * This header is the only place that decides where fixed-width types come
 * from.  Everything else in the library includes this instead of <stdint.h>
 * or STL headers, which makes the decode core compilable as-is by:
 *   - a hosted C++17/C++20 compiler (user space),
 *   - MSVC with /kernel (Windows driver, no CRT),
 *   - gcc/clang with -ffreestanding -nostdinc++ (Linux kernel module).
 *
 * Kernel callers must include the platform headers first (<ntddk.h> for
 * Windows drivers, or build with kbuild's include path for Linux).
 *
 * Two rules the core follows because of what kernel headers do to C++:
 *   - no `auto`: <linux/compiler_types.h> defines `auto` as `__auto_type`, which
 *     breaks trailing return types and deduced locals.
 *   - no `inline` on namespace-scope variables: the same header expands `inline`
 *     to `inline __gnu_inline`, and that attribute is invalid on objects.
 *     (A namespace-scope `constexpr` array already has internal linkage.)
 */
#ifndef LENDIZA_KERNEL_H
#define LENDIZA_KERNEL_H

#if defined(__KERNEL__) /* Linux kernel (kbuild) */

#include <linux/types.h>
#include <linux/stddef.h>

typedef __u8 ldz_u8;
typedef __u16 ldz_u16;
typedef __u32 ldz_u32;
typedef __u64 ldz_u64;
typedef size_t ldz_size;

#define LENDIZA_ENV_KERNEL_LINUX 1

#elif defined(_KERNEL_MODE) /* Windows kernel (WDK, /kernel) */

/* cl predefines _KERNEL_MODE for /kernel, so this header works standalone as
 * long as the WDK include path is available: ntdef.h carries UINT8/SIZE_T. */
#ifndef _NTDEF_
#include <ntdef.h>
#endif

typedef UINT8 ldz_u8;
typedef UINT16 ldz_u16;
typedef UINT32 ldz_u32;
typedef UINT64 ldz_u64;
typedef SIZE_T ldz_size;

#define LENDIZA_ENV_KERNEL_WINDOWS 1

#else /* hosted */

#include <stddef.h>
#include <stdint.h>

typedef uint8_t ldz_u8;
typedef uint16_t ldz_u16;
typedef uint32_t ldz_u32;
typedef uint64_t ldz_u64;
typedef size_t ldz_size;

#endif

/* Longest possible x86 instruction, prefixes included.  Kernel callers should
 * hand the decoder min(LDZ_MAX_INSNS_LEN, readable bytes) so a complete
 * instruction always fits, and must pass memory that is safe to read at the
 * current IRQL (copy user buffers in first - the decoder never probes). */
#define LDZ_MAX_INSNS_LEN 15u

/* Error results share the return type with lengths: any value >= LDZ_ERR_BASE
 * is an error code, never a length. */
#define LDZ_ERR_BASE 0xE0u
#define LDZ_ERR_INSUFFICIENT_BUFFER 0xE0u
#define LDZ_ERR_UNDEFINED_INSTRUCTION 0xE1u
#define LDZ_ERR_UNKNOWN 0xE2u

#endif /* LENDIZA_KERNEL_H */
