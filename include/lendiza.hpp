#ifndef LENDIZA_H
#define LENDIZA_H

/* Hosted convenience layer: std::span overloads and the polymorphic interface.
 * Kernel code must include lendiza_core.hpp instead - this header needs <span>. */

#include "lendiza_core.hpp"

#include <span>

namespace lendiza
{

struct ldiza {
    virtual size_t ldisasm(std::span<const uint8_t> buffer) const = 0;
    virtual ~ldiza() = default;
};

template<size_t N>
struct ldiza_x86 : public ldiza {
    static_assert(N == 32 || N == 64,
                  "ldiza_x86 is only supported for N == 32 or N == 64");
};

template<>
struct ldiza_x86<32> : public ldiza {
    /* Prefix bytes belong to the instruction they precede, so the returned
     * length covers them as well; a buffer that cannot hold the whole encoding
     * yields LENDIZA_ERR_INSUFFICIENT_BUFFER rather than a guessed length. */
    auto operator()(std::span<const uint8_t> buffer) const -> size_t
    {
        return detail::x86traits::ldiza(buffer.data(), buffer.size());
    }

    size_t ldisasm(std::span<const uint8_t> buffer) const override
    {
        return (*this)(buffer);
    }
};

template<>
struct ldiza_x86<64> : public ldiza {
    auto operator()(std::span<const uint8_t> buffer) const noexcept -> size_t
    {
        return detail::amd64traits::ldiza(buffer.data(), buffer.size());
    }

    size_t ldisasm(std::span<const uint8_t> buffer) const override
    {
        return (*this)(buffer);
    }
};

} // !namespace lendiza

#endif // !LENDIZA_H
