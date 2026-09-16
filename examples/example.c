#include "lendiza_export.h"
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    /* Simple code buffer: NOP, NOP, RET */
    const uint8_t code[] = { 0x90, 0x90, 0xC3 };
    size_t offset = 0;

    while (offset < sizeof(code)) {
        const size_t len = lendiza_disasm_x64(code + offset, sizeof(code) - offset);
        if (len >= LENDIZA_ERR_INSUFFICIENT_BUFFER) {
            fprintf(stderr, "decode error at %zu: %zu\n", offset, len);
            return 1;
        }
        printf("instr at %zu len=%zu\n", offset, len);
        offset += len;
    }

    return 0;
}
