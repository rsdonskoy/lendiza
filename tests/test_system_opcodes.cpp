// test_system_opcodes.cpp
// Control, model-specific and I/O encodings in both modes: the forms a kernel
// scanner meets when it walks privileged code.
//
// These are mostly short fixed-length encodings, so the suite concentrates on
// what is easy to get wrong: which forms carry a ModRM, which carry an
// immediate, what happens at the end of the buffer, and which slots are only
// defined in one of the two modes (a scanner that confuses them desyncs).
//
// Suites: SystemOpcodeTest (x64), SystemOpcodeTest32 (IA-32).

#include <cstdint>
#include <vector>

#include "lendiza_decode_helpers.h"
#include "lendiza_test.h"

using namespace lzdh;

TEST(SystemOpcodeTest, Control_register_transfers_are_three_bytes)
{
    static const uint8_t cr[] = { 0x20, 0x21, 0x22, 0x23 }; /* 0F 26/27 (task regs) are undefined */
    for (const uint8_t second : cr) {
        EXPECT_EQ(x64({ 0x0F, second, 0xC1 }), 3u);       /* MOV CR0, r64 */
        EXPECT_EQ(x64({ 0x0F, second, 0x00 }), 3u);       /* mod!=11 has no memory form, */
        EXPECT_EQ(x64({ 0x0F, second, 0x40, 0x10 }), 3u); /* so the map stays at 3 bytes */
        EXPECT_EQ(x64({ 0x48, 0x0F, second, 0xC1 }), 4u); /* REX joins the instruction */
        EXPECT_EQ(x64({ 0x0F, second }), ERR_INSUFFICIENT);
    }
    /* 0F 24/25 are the retired test-register moves in long mode. */
    EXPECT_EQ(x64({ 0x0F, 0x24, 0xC1 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0x0F, 0x25, 0xC1 }), ERR_UNDEFINED);
}

TEST(SystemOpcodeTest, No_operand_control_forms_are_two_bytes)
{
    static const uint8_t simple[] = { 0x05, 0x06, 0x07, 0x08, 0x09, 0x0B, 0x30, 0x31, 0x32,
                                      0x34, 0x35, 0xA2 };
    for (const uint8_t second : simple) {
        EXPECT_EQ(x64({ 0x0F, second }), 2u);
        EXPECT_EQ(x64({ 0x66, 0x0F, second }), 3u);
        EXPECT_EQ(x64({ 0xF3, 0x0F, second }), 3u);
    }
    EXPECT_EQ(x64({ 0x0F }), ERR_INSUFFICIENT);
    EXPECT_EQ(x64({ 0x0F, 0x0D, 0xC0 }), 3u); /* PREFETCH group takes a ModRM */
}

TEST(SystemOpcodeTest, One_byte_system_ops)
{
    static const uint8_t single[] = { 0x9C, 0x9D, 0xF4, 0xFA, 0xFB, 0xFC, 0xFD };
    for (const uint8_t opc : single) {
        EXPECT_EQ(x64({ opc }), 1u);
        EXPECT_EQ(x64({ 0x64, opc }), 2u); /* a segment override joins the instruction */
    }
}

TEST(SystemOpcodeTest, Flag_and_io_port_forms)
{
    EXPECT_EQ(x64({ 0xF8 }), 1u);          /* CLC */
    EXPECT_EQ(x64({ 0xF5 }), 1u);          /* CMC */
    EXPECT_EQ(x64({ 0xCC }), 1u);           /* INT3 */
    EXPECT_EQ(x64({ 0xCD, 0x2E }), 2u);     /* INT imm8 */
    EXPECT_EQ(x64({ 0xE4, 0x60 }), 2u);     /* IN al, imm8 */
    EXPECT_EQ(x64({ 0xE5, 0x60 }), 2u);     /* IN eAX, imm8 */
    EXPECT_EQ(x64({ 0xEC }), 1u);           /* IN al, DX */
    EXPECT_EQ(x64({ 0xED }), 1u);           /* IN eAX, DX */
    EXPECT_EQ(x64({ 0x66, 0xED }), 2u);
    EXPECT_EQ(x64({ 0xE6, 0x60 }), 2u);     /* OUT imm8, al */
    EXPECT_EQ(x64({ 0xEE }), 1u);           /* OUT DX, al */
    EXPECT_EQ(x64({ 0xE4 }), ERR_INSUFFICIENT); /* the port byte is missing */
}

TEST(SystemOpcodeTest, Lgdt_sidt_memory_forms)
{
    /* 0F 01 /2-/3 (lgdt/lidt) and /4-/6 (sgdt/sidt) take a memory operand, so the
     * displacement must be counted - the case this refactor fixed. */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x10 }), 3u);                /* [rax] */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x50, 0x10 }), 4u);          /* [rax+0x10] */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x90, 1, 2, 3, 4 }), 7u);    /* [rax+disp32] */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x24, 0x00 }), 4u);          /* SIB, no displacement */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x25, 1, 2, 3, 4 }), 7u);    /* rip-relative */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x94, 0x08, 1, 2, 3, 4 }), 8u); /* SIB + disp32 */
}

TEST(SystemOpcodeTest, Group7_and_reserved_members)
{
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x30 }), 3u); /* lmsw m16 */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0xF8 }), 3u); /* register-only extension */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0xF9 }), 3u);
    EXPECT_EQ(x64({ 0x0F, 0xC7, 0xC8 }), 3u); /* cmpxchg8b/16b register form */
    EXPECT_EQ(x64({ 0x0F, 0xC7, 0x00 }), 3u); /* cmpxchg8b [rax] */
    EXPECT_EQ(x64({ 0x0F, 0xB9, 0xC0 }), 3u); /* 0F B9 group */
    EXPECT_EQ(x64({ 0x0F, 0x01, 0x24 }), ERR_INSUFFICIENT); /* SIB byte missing */
}

TEST(SystemOpcodeTest32, Push_pop_segment_and_far_returns_exist)
{
    EXPECT_EQ(x86({ 0x0F, 0xA0 }), 2u);              /* PUSH FS */
    EXPECT_EQ(x86({ 0x0F, 0xA1 }), 2u);              /* POP FS */
    EXPECT_EQ(x86({ 0x0F, 0xA8 }), 2u);               /* PUSH GS */
    EXPECT_EQ(x86({ 0x0F, 0xA9 }), 2u);               /* POP GS */
    EXPECT_EQ(x86({ 0x06 }), 1u);                    /* PUSH ES */
    EXPECT_EQ(x86({ 0x0E }), 1u);                    /* PUSH CS */
    EXPECT_EQ(x86({ 0x0F, 0x08 }), 2u);              /* INVD */
    EXPECT_EQ(x86({ 0x0F, 0x20, 0xC1 }), 3u);        /* MOV CR0, eax */
    EXPECT_EQ(x86({ 0xEA, 1, 2, 3, 4, 5, 6 }), 7u);  /* JMP ptr16:32 */
    EXPECT_EQ(x86({ 0xCB }), 1u);                    /* RETF */
    EXPECT_EQ(x86({ 0xCF }), 1u);                    /* IRET */
    EXPECT_EQ(x86({ 0x9D }), 1u);                    /* POPFD */

    /* Long mode differences: PUSH/POP FS still exist, but PUSH ES and far JMP do not. */
    EXPECT_EQ(x64({ 0x0F, 0xA0 }), 2u);
    EXPECT_EQ(x64({ 0x06 }), ERR_UNDEFINED);
    EXPECT_EQ(x64({ 0xEA, 1, 2, 3, 4, 5, 6 }), ERR_UNDEFINED);
}

TEST(SystemOpcodeTest32, Msr_and_rdtsc_pairs_match_long_mode)
{
    static const uint8_t simple[] = { 0x30, 0x31, 0x32, 0x34, 0x35, 0x06, 0x08, 0x09 };
    for (const uint8_t second : simple) {
        EXPECT_EQ(x86({ 0x0F, second }), 2u);
        EXPECT_EQ(x64({ 0x0F, second }), 2u); /* same length in both modes */
    }
    EXPECT_EQ(x86({ 0x0F, 0x04 }), ERR_UNDEFINED);
    EXPECT_EQ(x86({ 0x0F, 0x0C }), ERR_UNDEFINED);
}

TEST(SystemOpcodeTest, Row_a_lengths_agree_across_modes)
{
    /* 0F A0-0F AF encodes the same instructions in both modes, so a table row that
     * differs between them is a bug, not a mode difference.  The IA-32 row used to
     * carry 0x06 ("six bytes") across A8-AF, which no single-mode test would catch. */
    struct Row {
        uint8_t second;
        size_t with_modrm;  /* length of 0F xx 0xC1 (mod=11 register form) */
        const char* what;
    };
    static const Row rows[] = {
        { 0xA0, 2, "PUSH FS" },
        { 0xA1, 2, "POP FS" },
        { 0xA2, 2, "CPUID" },
        { 0xA3, 3, "BT r/m, r" },
        { 0xA4, 4, "SHLD r/m, r, imm8" },
        { 0xA5, 3, "SHLD r/m, r, CL" },
        { 0xA8, 2, "PUSH GS" },
        { 0xA9, 2, "POP GS" },
        { 0xAA, 2, "RSM" },
        { 0xAB, 3, "BTS r/m, r" },
        { 0xAC, 4, "SHRD r/m, r, imm8" },
        { 0xAD, 3, "SHRD r/m, r, CL" },
        { 0xAF, 3, "IMUL r, r/m" },
    };
    /* One trailing byte beyond the imm8 rows, so the fixed-length entries above are
     * simply not fed to the end; the decoder must stop at each instruction's own
     * boundary either way. */
    for (const Row& r : rows) {
        EXPECT_EQ(x64({ 0x0F, r.second, 0xC1, 0x00 }), r.with_modrm);
        EXPECT_EQ(x86({ 0x0F, r.second, 0xC1, 0x00 }), r.with_modrm);
    }

    /* A displacement is counted on top of the immediate: 2 opcode + ModRM +
     * disp32 + imm8 = 8. */
    EXPECT_EQ(x64({ 0x0F, 0xA4, 0x80, 1, 2, 3, 4, 0x08 }), 8u); /* SHLD [eax+disp32], eax, 8 */
    EXPECT_EQ(x86({ 0x0F, 0xA4, 0x80, 1, 2, 3, 4, 0x08 }), 8u);
    EXPECT_EQ(x86({ 0x0F, 0xAC, 0x40, 0x10, 0x08 }), 5u);        /* SHRD [eax+0x10], eax, 8 */
    EXPECT_EQ(x86({ 0x0F, 0xAC, 0x80, 1, 2, 3, 4 }), ERR_INSUFFICIENT); /* its imm8 is missing */
    EXPECT_EQ(x64({ 0x0F, 0xAF, 0x05, 1, 2, 3, 4 }), 7u);         /* IMUL eax, [rip+disp32] */
}
