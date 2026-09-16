// lendiza_code_corpus.h - C++ view of the shared code corpus.
#ifndef LENDIZA_CODE_CORPUS_H
#define LENDIZA_CODE_CORPUS_H

#include "lendiza_code_corpus_c.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>

namespace lzc {

inline const uint8_t* shellcode(size_t& n)
{
    n = LENDIZA_CODE_CORPUS_LEN;
    return lendiza_code_corpus;
}

} // namespace lzc
#endif /* __cplusplus */

#endif // LENDIZA_CODE_CORPUS_H
