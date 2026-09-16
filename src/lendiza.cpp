#include "lendiza_core.hpp"
#include "lendiza_export.h"

extern "C" {

size_t lendiza_disasm_x86(const uint8_t* buffer, size_t length)
{
    if (buffer == nullptr || length == 0) {
        return LENDIZA_ERR_INSUFFICIENT_BUFFER;
    }

    return lendiza::detail::x86traits::ldiza(buffer, length);
}

size_t lendiza_disasm_x64(const uint8_t* buffer, size_t length)
{
    if (buffer == nullptr || length == 0) {
        return LENDIZA_ERR_INSUFFICIENT_BUFFER;
    }

    return lendiza::detail::amd64traits::ldiza(buffer, length);
}

} // extern "C"
