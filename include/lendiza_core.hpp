/* lendiza_core.hpp - kernel-safe umbrella.
 *
 * Include this from a Windows driver (cl /kernel /GR- /EHs-c- /std:c++17) or a
 * Linux kernel module (g++ -ffreestanding -nostdinc++ -fno-exceptions
 * -fno-rtti).  It pulls in no STL, throws nothing, dispatches through no
 * vtable and keeps no mutable global state, so it is safe to call at
 * DISPATCH_LEVEL or from interrupt context.
 *
 * Do not include lendiza.hpp from kernel code: that header is the hosted
 * convenience layer (std::span and the polymorphic interface).
 */
#ifndef LENDIZA_CORE_H
#define LENDIZA_CORE_H

#include "lendiza_kernel.h"
#include "lendiza_common.hpp"
#include "lendiza_x86.hpp"
#include "lendiza_amd64.hpp"

#ifdef __cplusplus

namespace lendiza
{

/* Length of the instruction at buffer[0] in 64-bit mode, or an LDZ_ERR_* code.
 * buffer[0..length-1] must be readable; pass min(LDZ_MAX_INSNS_LEN, readable)
 * bytes.  In a driver, copy user-mode memory in before calling. */
inline ldz_size disasm_x64(const ldz_u8* buffer, ldz_size length) noexcept
{
    return detail::amd64traits::ldiza(buffer, length);
}

/* Same for IA-32 (32-bit) code. */
inline ldz_size disasm_x86(const ldz_u8* buffer, ldz_size length) noexcept
{
    return detail::x86traits::ldiza(buffer, length);
}

/* True when a decode result is an error rather than an instruction length. */
inline bool is_error(ldz_size result) noexcept
{
    return detail::is_error(result);
}

} // namespace lendiza

#endif /* __cplusplus */

#endif // !LENDIZA_CORE_H
