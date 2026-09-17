#ifndef LENDIZA_AMD64_H
#define LENDIZA_AMD64_H

#include "lendiza_common.hpp"
#include "lendiza_amd64_tables.hpp"

namespace lendiza::detail
{

struct amd64traits {

    enum class one_byte_opcode {
        ADD_Eb_Gb = 0x00,
        ADD_Ev_Gv = 0x01,
        ADD_Gb_Eb = 0x02,
        ADD_Gv_Ev = 0x03,
        ADD_AL_Ib = 0x04,
        ADD_eAX_Iv = 0x05,
        UD_06 = 0x06,
        UD_07 = 0x07,
        OR_Eb_Gb = 0x08,
        OR_Ev_Gv = 0x09,
        OR_Gb_Eb = 0x0A,
        OR_Gv_Ev = 0x0B,
        OR_AL_Ib = 0x0C,
        OR_rAX_Iz = 0x0D,
        UD_0E = 0x0E,
        _2_BYTE_ESC = 0x0F,
        ADC_Eb_Gb = 0x10,
        ADC_Ev_Gv = 0x11,
        ADC_Gb_Eb = 0x12,
        ADC_Gv_Ev = 0x13,
        ADC_AL_Ib = 0x14,
        ADC_rAX_Iz = 0x15,
        UD_16 = 0x16,
        UD_17 = 0x17,
        SBB_Eb_Gb = 0x18,
        SBB_Ev_Gv = 0x19,
        SBB_Gb_Eb = 0x1A,
        SBB_Gv_Ev = 0x1B,
        SBB_AL_Ib = 0x1C,
        SBB_rAX_Iz = 0x1D,
        UD_1E = 0x1E,
        UD_1F = 0x1F,
        AND_Eb_Gb = 0x20,
        AND_Ev_Gv = 0x21,
        AND_Gb_Eb = 0x22,
        AND_Gv_Ev = 0x23,
        AND_AL_Ib = 0x24,
        AND_rAX_Iz = 0x25,
        PREFIX_SEG_ES = 0x26,
        UD_27 = 0x27,
        SUB_Eb_Gb = 0x28,
        SUB_Ev_Gv = 0x29,
        SUB_Gb_Eb = 0x2A,
        SUB_Gv_Ev = 0x2B,
        SUB_AL_Ib = 0x2C,
        SUB_rAX_Iz = 0x2D,
        PREFIX_SEG_CS = 0x2E,
        UD_2F = 0x2F,
        XOR_Eb_Gb = 0x30,
        XOR_Ev_Gv = 0x31,
        XOR_Gb_Eb = 0x32,
        XOR_Gv_Ev = 0x33,
        XOR_AL_Ib = 0x34,
        XOR_rAX_Iz = 0x35,
        PREFIX_REG_SS = 0x36,
        CMP_Eb_Gb = 0x38,
        CMP_Ev_Gv = 0x39,
        CMP_Gb_Eb = 0x3A,
        CMP_Gv_Ev = 0x3B,
        CMP_AL_Ib = 0x3C,
        CMP_rAX_Iz = 0x3D,
        PREFIX_SEG_DS = 0x3E,
        UD_3F = 0x3F,
        PREFIX_REX = 0x40,
        PREFIX_REX_B = 0x41,
        PREFIX_REX_X = 0x42,
        PREFIX_REX_XB = 0x43,
        PREFIX_REX_R = 0x44,
        PREFIX_REX_RB = 0x45,
        PREFIX_REX_RX = 0x46,
        PREFIX_REX_RXB = 0x47,
        PREFIX_REX_W = 0x48,
        PREFIX_REX_WB = 0x49,
        PREFIX_REX_WX = 0x4A,
        PREFIX_REX_WXB = 0x4B,
        PREFIX_REX_WR = 0x4C,
        PREFIX_REX_WRB = 0x4D,
        PREFIX_REX_WRX = 0x4E,
        PREFIX_REX_WRXB = 0x4F,
        PUSH_rAX = 0x50,
        PUSH_rCX = 0x51,
        PUSH_rDX = 0x52,
        PUSH_rBX = 0x53,
        PUSH_rSP = 0x54,
        PUSH_rBP = 0x55,
        PUSH_rSI = 0x56,
        PUSH_rDI = 0x57,
        POP_rAX = 0x58,
        POP_rCX = 0x59,
        POP_rDX = 0x5A,
        POP_rBX = 0x5B,
        POP_rSP = 0x5C,
        POP_rBP = 0x5D,
        POP_rSI = 0x5E,
        POP_rDI = 0x5F,
        UD_60 = 0x60,
        UD_61 = 0x61,
        UD_62 = 0x62,
        MOVSXD_Gv_Ev = 0x63,
        PREFIX_SEG_FS = 0x64,
        PREFIX_SEG_GS = 0x65,
        PREFIX_OPD_SIZE = 0x66,
        PREFIX_ADDR_SIZE = 0x67,
        PUSH_Iz = 0x68,
        IMUL_Gv_Ev_Iz = 0x69,
        PUSH_Ib = 0x6A,
        IMUL_Gv_Ev_Ib = 0x6B,
        INSB = 0x6C,
        INSD = 0x6D,
        OUTSB = 0x6E,
        OUTSD = 0x6F,
        JO = 0x70,
        JNO = 0x71,
        JC = 0x72,
        JNC = 0x73,
        JZ = 0x74,
        JNZ = 0x75,
        JBE = 0x76,
        JA = 0x77,
        JS = 0x78,
        JNS = 0x79,
        JP = 0x7A,
        JNP = 0x7B,
        JL = 0x7C,
        JNL = 0x7D,
        JLE = 0x7E,
        JG = 0x7F,
        IMM_GRP1_Eb_Ib = 0x80,
        IMM_GRP1_Ev_Iz = 0x81,
        UD_82 = 0x82,
        IMM_GRP1_Ev_Ib = 0x83,
        TEST_Eb_Gb = 0x84,
        TEST_Ev_Gv = 0x85,
        XCHG_Eb_Gb = 0x86,
        XCHG_Ev_Gv = 0x87,
        MOV_Eb_Gb = 0x88,
        MOV_Ev_Gv = 0x89,
        MOV_Gb_Eb = 0x8A,
        MOV_Gv_Ev = 0x8B,
        MOV_Ev_Sw = 0x8C,
        LEA_Gv_M = 0x8D,
        MOV_Sw_Ew = 0x8E,
        POP_Ev = 0x8F,
        NOP = 0x90,
        XCHG_eCX_eAX = 0x91,
        XCHG_eDX_eAX = 0x92,
        XCHG_eBX_eAX = 0x93,
        XCHG_eSP_eAX = 0x94,
        XCHG_eBP_eAX = 0x95,
        XCHG_eSI_eAX = 0x96,
        XCHG_eDI_eAX = 0x97,
        CWDE = 0x98,
        CDQ = 0x99,
        UD_9A = 0x9A,
        FWAIT = 0x9B,
        PUSHFQ = 0x9C,
        POPFQ = 0x9D,
        SAHF = 0x9E,
        LAHF = 0x9F,
        MOV_AL_Ob = 0xA0,
        MOV_rAX_Ov = 0xA1,
        MOV_Ob_AL = 0xA2,
        MOV_Ov_rAX = 0xA3,
        MOVSB = 0xA4,
        MOVSD = 0xA5,
        CMPSB = 0xA6,
        CMPSD = 0xA7,
        TEST_AL_Ib = 0xA8,
        TEST_rAX_Iz = 0xA9,
        STOSB = 0xAA,
        STOSD = 0xAB,
        LODSB = 0xAC,
        LODSD = 0xAD,
        SCASB = 0xAE,
        SCASD = 0xAF,
        MOV_AL_Ib = 0xB0,
        MOV_CL_Ib = 0xB1,
        MOV_DL_Ib = 0xB2,
        MOV_BL_Ib = 0xB3,
        MOV_AH_Ib = 0xB4,
        MOV_CH_Ib = 0xB5,
        MOV_DH_Ib = 0xB6,
        MOV_BH_Ib = 0xB7,
        MOV_rAX_Iv = 0xB8,
        MOV_rCX_Iv = 0xB9,
        MOV_rDX_Iv = 0xBA,
        MOV_rBX_Iv = 0xBB,
        MOV_rSP_Iv = 0xBC,
        MOV_rBP_Iv = 0xBD,
        MOV_rSI_Iv = 0xBE,
        MOV_rDI_Iv = 0xBF,
        SHIFT_GRP2_Eb_Ib = 0xC0,
        SHIFT_GRP2_Ev_Ib = 0xC1,
        RET_Iw = 0xC2,
        RET = 0xC3,
        PREFIX_VEX_C4 = 0xC4,
        PREFIX_VEX_C5 = 0xC5,
        MOV_Eb_Ib = 0xC6,
        MOV_Ev_Iz = 0xC7,
        ENTER_Iw_Ib = 0xC8,
        LEAVE = 0xC9,
        RETF_Iw = 0xCA,
        RETF = 0xCB,
        INT3 = 0xCC,
        INT_Ib = 0xCD,
        UD_CE = 0xCE,
        IRETD = 0xCF,
        SHIFT_GRP_2_Eb_1 = 0xD0,
        SHIFT_GRP_2_Ev_1 = 0xD1,
        SHIFT_GRP_2_Eb_CL = 0xD2,
        SHIFT_GRP_2_Ev_CL = 0xD3,
        UD_D4 = 0xD4,
        UD_D5 = 0xD5,
        UD_D6 = 0xD6,
        XLATB = 0xD7,
        X87_ESC_D8 = 0xD8,
        X87_ESC_D9 = 0xD9,
        X87_ESC_DA = 0xDA,
        X87_ESC_DB = 0xDB,
        X87_ESC_DC = 0xDC,
        X87_ESC_DD = 0xDD,
        X87_ESC_DE = 0xDE,
        X87_ESC_DF = 0xDF,
        LOOPNE_Jb = 0xE0,
        LOOPE_Jb = 0xE1,
        LOOP_Jb = 0xE2,
        JrCXZ_Jb = 0xE3,
        IN_AL_Ib = 0xE4,
        IN_eAX_Ib = 0xE5,
        OUT_Ib_AL = 0xE6,
        OUT_Ib_eAX = 0xE7,
        CALL_Jz = 0xE8,
        JMP_Jz = 0xE9,
        UD_EA = 0xEA,
        JMP_Jb = 0xEB,
        IN_AL_DX = 0xEC,
        IN_eAX_DX = 0xED,
        OUT_DX_AL = 0xEE,
        OUT_DX_eAX = 0xEF,
        PREFIX_LOCK = 0xF0,
        INT_1 = 0xF1,
        PREFIX_REPNE = 0xF2,
        PREFIX_REP = 0xF3,
        HLT = 0xF4,
        CMC = 0xF5,
        UNARY_GRP3_Eb = 0xF6,
        UNARY_GRP3_Ev = 0xF7,
        CLC = 0xF8,
        CTC = 0xF9,
        CLI = 0xFA,
        STI = 0xFB,
        CLD = 0xFC,
        STD = 0xFD,
        INC_DEC_GRP_4 = 0xFE,
        INC_DEC_GRP_5 = 0xFF
    };

    /* Length of the instruction at p[0], or an LDZ_ERR_* code.
     *
     * p[0..n-1] must be readable.  When the buffer cannot hold the whole
     * encoding the result is LDZ_ERR_INSUFFICIENT_BUFFER: the decoder never
     * reads past n and never guesses a length, which is what makes it callable
     * from a driver at DISPATCH_LEVEL.  Hand it min(LDZ_MAX_INSNS_LEN, readable)
     * bytes.  Memory referenced by p is read as-is; a caller in kernel mode
     * must copy user buffers in first.
     */
    static ldz_size ldiza(const ldz_u8* p, ldz_size n)
    {
        if (p == nullptr || n == 0) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        const cursor c { p, n };
        prefix_info pf {};
        ldz_size first = 0;

        const ldz_size prefix_err = process_prefix(c, pf, first);
        if (prefix_err != 0) {
            return prefix_err;
        }

        const ldz_size body = decode(c, pf, first);
        if (is_error(body)) {
            return body;
        }

        const ldz_size total = pf.length + body;
        if (total > LDZ_MAX_INSNS_LEN) {
            return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);
        }
        /* The branches above only check the bytes they interpret; an immediate
         * can still push the instruction past what the caller guarantees is
         * readable.  Report that instead of returning a guessed length. */
        if (!c.has(total)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }
        return total;
    }

private:
    /* Legacy prefixes belong to the instruction they precede, so their bytes are
     * part of the returned length.  VEX/EVEX is reported as unimplemented, the
     * same call xendiza makes. */
    enum prefix_byte : ldz_u8 {
        pfx_none = 0,
        pfx_66,
        pfx_67,
        pfx_lock,
        pfx_rep,
        pfx_seg,
        pfx_rex,
        pfx_opc_2byte,
        pfx_vex2,
        pfx_vex3,
        pfx_evex
    };

    static constexpr ldz_u8 k_prefix_table[256] = {
        /*      0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
        /* 0 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_opc_2byte,
        /* 1 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 2 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_none,
        /* 3 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_none,
        /* 4 */ pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,  pfx_rex,
        /* 5 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 6 */ pfx_none, pfx_none, pfx_evex, pfx_none, pfx_seg,  pfx_seg,  pfx_66,   pfx_67,   pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 7 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 8 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 9 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* A */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* B */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* C */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_vex3, pfx_vex2, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* D */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* E */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* F */ pfx_lock, pfx_none, pfx_rep,  pfx_rep,  pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none
    };

    /* Trailing immediate/relative field per opcode, i.e. what 0x66 and 0x67 can
     * resize.  tail_none means the length map already holds the answer. */
    static constexpr ldz_u8 k_tail_table[256] = {
        /*      0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
        /* 0 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none,
        /* 1 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none,
        /* 2 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none,
        /* 3 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none,
        /* 4 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 5 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 6 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_iz,   tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 7 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 8 */ tail_none, tail_iz,  tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 9 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* A */ tail_moffs, tail_moffs, tail_moffs, tail_moffs, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* B */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iq,   tail_iq,   tail_iq,   tail_iq,   tail_iq,   tail_iq,   tail_iq,   tail_iq,
        /* C */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* D */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* E */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_relz, tail_relz, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* F */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none
    };

    /* 0x0F-prefixed opcodes with a resizable trailing field. */
    static constexpr ldz_u8 k_tail_table_2b[256] = {
        /*      0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
        /* 0 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 1 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 2 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 3 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 4 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 5 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 6 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 7 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 8 */ tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz, tail_relz,
        /* 9 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* A */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* B */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* C */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* D */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* E */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* F */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none
    };

    static ldz_size process_prefix(const cursor& c, prefix_info& pf, ldz_size& first)
    {
        while (first < c.size() && first < LDZ_MAX_INSNS_LEN) {
            const ldz_u8 byte = c[first];
            const ldz_u8 kind = k_prefix_table[byte];
            switch (kind) {
            case pfx_none:
            case pfx_opc_2byte:
                return 0; /* 0x0F is an opcode escape, not a consumed prefix */

            case pfx_rex:
                pf.rex_byte = byte; /* repeats are legal; the last one wins */
                break;

            /* Repeating a legacy prefix is not an error: the CPU keeps one slot
             * per prefix type, so a later byte overwrites the earlier one and
             * every byte still counts toward the instruction length.  Measured
             * on hardware against the 15-byte cap in ldiza(), which is the only
             * limit that actually applies. */
            case pfx_66:
                pf.has_66 = 1;
                break;

            case pfx_67:
                pf.has_67 = 1;
                break;

            case pfx_lock:
                pf.has_lock = 1;
                break;

            case pfx_rep:
                pf.has_rep = 1;
                break;

            case pfx_seg: /* a later segment override replaces the previous one */
                pf.has_seg = 1;
                break;

            case pfx_vex2:
            case pfx_vex3:
            case pfx_evex:
                return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);

            default:
                return LDZ_ERR_CODE(UNKNOWN_ERROR);
            }

            /* Only the REX sitting immediately before the opcode counts; any
             * legacy prefix consumed after it voids that byte outright, its
             * W/X/B/R bits included, since rex_w() and friends read rex_byte. */
            if (kind != pfx_rex) {
                pf.rex_byte = 0;
            }

            ++pf.length;
            ++first;
        }

        if (first >= LDZ_MAX_INSNS_LEN) {
            return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION); /* prefixes alone overflow */
        }
        return LDZ_ERR_CODE(INSUFFICIENT_BUFFER); /* prefixes, no opcode byte */
    }

    static ldz_size decode(const cursor& c, const prefix_info& pf, const ldz_size first)
    {
        const ldz_u8 opc = c[first];
        switch (const ldz_u8 mapped = kLengthTable_1byte_opc[opc]) {
        case 0x00: /* ModRM, SIB and displacement follow the opcode */
            return modrm_len(c, first, 1, pf);

        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
            return 1 + tail_width(k_tail_table[opc], mapped - 1, pf);

        case 0xA1: { /* ModRM plus an imm8 that the length map does not count */
            const ldz_size body = modrm_len(c, first, 1, pf);
            return is_error(body) ? body : body + SIZE_1_BYTE_IMMEDIATE;
        }

        case 0xA4: { /* ModRM plus an immediate sized by 0x66 */
            const ldz_size body = modrm_len(c, first, 1, pf);
            if (is_error(body)) {
                return body;
            }
            return body + tail_width(k_tail_table[opc], SIZE_4_BYTE_IMMEDIATE, pf);
        }

        case 0xB8: /* MOV r64, imm64 under REX.W; imm16 under 0x66 */
            return 1 + tail_width(k_tail_table[opc], SIZE_4_BYTE_IMMEDIATE, pf);

        case 0xE2:
            return process_2byte_opc(c, first, pf);

        case 0xEE: {
            const ldz_u8 (*table)[256] = pick_grp1_table(opc);
            if (table == nullptr) {
                return LDZ_ERR_CODE(UNKNOWN_ERROR);
            }
            return group_len(c, first, 1, pf, *table, &modrm_len, &tail_width);
        }

        case 0xEF:
            return group_len(c, first, 1, pf, kLengthTable_x87[opc - 0xD8], &modrm_len, &tail_width);

        case 0xFF:
            return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);

        default:
            /* 0xA0 used to mean "this byte is a REX prefix"; prefixes are now
             * consumed above, so it is unreachable. */
            return LDZ_ERR_CODE(UNKNOWN_ERROR);
        }
    }

    static ldz_size tail_width(const ldz_u8 kind, const ldz_size default_width, const prefix_info& pf)
    {
        switch (kind) {
        case tail_iz:
            return imm_z_width(pf.has_66 != 0);
        case tail_iq:
            return imm_q_width(pf.has_66 != 0, pf.rex_w());
        case tail_relz:
            return rel_z_width(pf.has_66 != 0);
        case tail_moffs:
            return moffs_width(pf.has_67 != 0, true);
        case tail_far:
            return pf.has_66 != 0 ? 4 : 6;
        case tail_ib:
            return SIZE_1_BYTE_IMMEDIATE;
        case tail_iw:
            return SIZE_2_BYTE_IMMEDIATE;
        default:
            return default_width;
        }
    }

    /* Bytes occupied by the opcode bytes, the ModRM byte, an optional SIB byte
     * and the displacement, counted from `first`.  pf is part of the callable
     * contract group_len imposes on both traits; long mode no longer reads it,
     * since nothing about the displacement depends on the prefixes. */
    static ldz_size modrm_len(const cursor& c, const ldz_size first, const ldz_size opc_size,
                              [[maybe_unused]] const prefix_info& pf)
    {
        if (!c.has(first + opc_size + 1)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        const ldz_u8 modrm = c[first + opc_size];

        /* LEA cannot name a register: `8D C0` through `8D FF` are rejected by the
         * CPU outright, whatever the registers hold (measured as
         * EXCEPTION_ILLEGAL_INSTRUCTION over the whole band), so there is no
         * length to report.  c[first] is the opcode whenever opc_size is 1 and
         * 0x0F otherwise, so this never fires on the escape maps.  A single check
         * rather than a table because 0x8D is the only 1-byte opcode in this
         * class; revisit if the hardware sweep finds more. */
        if (c[first] == 0x8D && (modrm & 0xC0u) == 0xC0u) {
            return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);
        }

        const ldz_size mapped = kLengthTable_ModRM[modrm];
        if (mapped != 0) {
            return opc_size + mapped - 1; /* the map counts one opcode byte */
        }

        /* mod=00 with r/m=100: a SIB byte follows, and it carries a disp32 only
         * when base=101.  That holds whatever REX.B says: measured on hardware,
         * `41 8D 04 25 <disp32>` consumes 8 bytes (REX.B selects r13 as the base
         * at mod=01, but it never removes this displacement). */
        if (!c.has(first + opc_size + 2)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        const ldz_u8 sib = c[first + opc_size + 1];
        const bool base_is_101 = test_bits123_for(sib, 0b101);
        const ldz_size disp = base_is_101 ? SIZE_4_BYTE_DISPLACEMENT : 0;

        return opc_size + 2 + disp;
    }

    static ldz_size process_2byte_opc(const cursor& c, const ldz_size first, const prefix_info& pf)
    {
        if (!c.has(first + 2)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        const ldz_u8 second = c[first + 1];

        switch (const ldz_u8 mapped = kLengthTable_2byte_opc[second]) {
        case 0x00:
            return modrm_len(c, first, 2, pf);

        case 0x02:
        case 0x03:
        case 0x06:
        case 0x07:
            /* The map already counts the trailing field; replace its width
             * rather than adding another one. */
            return 2 + tail_width(k_tail_table_2b[second], mapped - 2, pf);

        case 0xA1: {
            const ldz_size body = modrm_len(c, first, 2, pf);
            return is_error(body) ? body : body + SIZE_1_BYTE_IMMEDIATE;
        }

        case 0xE8:
            return process_3byte_opc(c, first, pf, kLengthTable_3byte_opc_0F38);

        case 0xEA:
            return process_3byte_opc(c, first, pf, kLengthTable_3byte_opc_0F3A);

        case 0xEE: {
            const ldz_u8 (*table)[256] = pick_grp2_table(second);
            if (table == nullptr) {
                return LDZ_ERR_CODE(UNKNOWN_ERROR);
            }
            return group_len(c, first, 2, pf, *table, &modrm_len, &tail_width);
        }

        case 0xFF:
            return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);

        default:
            return LDZ_ERR_CODE(UNKNOWN_ERROR);
        }
    }

    static ldz_size process_3byte_opc(const cursor& c, const ldz_size first, const prefix_info& pf,
                                 const ldz_u8 (&table)[256])
    {
        if (!c.has(first + 3)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        switch (table[c[first + 2]]) {
        case 0x00:
            return modrm_len(c, first, 3, pf);

        case 0xA1: {
            const ldz_size body = modrm_len(c, first, 3, pf);
            return is_error(body) ? body : body + SIZE_1_BYTE_IMMEDIATE;
        }

        case 0xFF:
            return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);

        default:
            return LDZ_ERR_CODE(UNKNOWN_ERROR);
        }
    }

    /* Long-mode one-byte opcode-extension groups: C6/C7/F6/F7/FE/FF (sentinel
     * 0xEE in kLengthTable_1byte_opc).  The null return, and with it the
     * caller's UNKNOWN_ERROR branch, is unreachable: the sentinel-domain
     * static_asserts of GroupTableTest (tests/test_group_tables.cpp) pin every
     * 0xEE cell of that map to an opcode listed below.  Do not delete them. */
    static const ldz_u8 (*pick_grp1_table(const ldz_u8 opc))[256]
    {
        switch (opc) {
        case 0xC6: return &kLengthTable_opcode_extension_0xC6;
        case 0xC7: return &kLengthTable_opcode_extension_0xC7;
        case 0xF6: return &kLengthTable_opcode_extension_0xF6;
        case 0xF7: return &kLengthTable_opcode_extension_0xF7;
        case 0xFE: return &kLengthTable_opcode_extension_0xFE;
        case 0xFF: return &kLengthTable_opcode_extension_0xFF;
        default:   return nullptr;
        }
    }

    /* Long-mode members of the 0F group: 0F 00/01/18/71/72/73/AE/B9/BA/C7
     * (sentinel 0xEE in kLengthTable_2byte_opc).  As above, the null return is
     * unreachable only while the sentinel-domain static_asserts of
     * GroupTableTest (tests/test_group_tables.cpp) hold - do not delete them. */
    static const ldz_u8 (*pick_grp2_table(const ldz_u8 second))[256]
    {
        switch (second) {
        case 0x00: return &kLengthTable_0F_00;
        case 0x01: return &kLengthTable_0F_01;
        case 0x18: return &kLengthTable_0F_18;
        case 0x71: return &kLengthTable_0F_71;
        case 0x72: return &kLengthTable_0F_72;
        case 0x73: return &kLengthTable_0F_73;
        case 0xAE: return &kLengthTable_0F_AE;
        case 0xB9: return &kLengthTable_0F_B9;
        case 0xBA: return &kLengthTable_0F_BA;
        case 0xC7: return &kLengthTable_0F_C7;
        default:   return nullptr;
        }
    }
};

} // namespace lendiza::detail

#define OPCODE_AMD64(x) (static_cast<ldz_u8>(lendiza::detail::amd64traits::one_byte_opcode::x))

#endif // !LENDIZA_AMD64_H
