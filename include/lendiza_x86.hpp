#ifndef LENDIZA_X86_H
#define LENDIZA_X86_H

#include "lendiza_common.hpp"
#include "lendiza_x86_tables.hpp"

namespace lendiza::detail
{

struct x86traits {

    enum class one_byte_opcode {
        ADD_Eb_Gb = 0x00,
        ADD_Ev_Gv = 0x01,
        ADD_Gb_Eb = 0x02,
        ADD_Gv_Ev = 0x03,
        ADD_AL_Ib = 0x04,
        ADD_eAX_Iv = 0x05,
        PUSH_ES = 0x06,
        POP_ES = 0x07,
        OR_Eb_Gb = 0x08,
        OR_Ev_Gv = 0x09,
        OR_Gb_Eb = 0x0A,
        OR_Gv_Ev = 0x0B,
        OR_AL_Ib = 0x0C,
        OR_eAX_Iv = 0x0D,
        PUSH_CS = 0x0E,
        _2_BYTE_ESC = 0x0F,
        ADC_Eb_Gb = 0x10,
        ADC_Ev_Gv = 0x11,
        ADC_Gb_Eb = 0x12,
        ADC_Gv_Ev = 0x13,
        ADC_AL_Ib = 0x14,
        ADC_eAX_Iv = 0x15,
        PUSH_SS = 0x16,
        POP_SS = 0x17,
        SBB_Eb_Gb = 0x18,
        SBB_Ev_Gv = 0x19,
        SBB_Gb_Eb = 0x1A,
        SBB_Gv_Ev = 0x1B,
        SBB_AL_Ib = 0x1C,
        SBB_eAX_Iv = 0x1D,
        PUSH_DS = 0x1E,
        POP_DS = 0x1F,
        AND_Eb_Gb = 0x20,
        AND_Ev_Gv = 0x21,
        AND_Gb_Eb = 0x22,
        AND_Gv_Ev = 0x23,
        AND_AL_Ib = 0x24,
        AND_eAX_Iv = 0x25,
        PREFIX_SEG_ES = 0x26,
        DAA = 0x27,
        SUB_Eb_Gb = 0x28,
        SUB_Ev_Gv = 0x29,
        SUB_Gb_Eb = 0x2A,
        SUB_Gv_Ev = 0x2B,
        SUB_AL_Ib = 0x2C,
        SUB_eAX_Iv = 0x2D,
        PREFIX_SEG_CS = 0x2E,
        DAS = 0x2F,
        XOR_Eb_Gb = 0x30,
        XOR_Ev_Gv = 0x31,
        XOR_Gb_Eb = 0x32,
        XOR_Gv_Ev = 0x33,
        XOR_AL_Ib = 0x34,
        XOR_eAX_Iv = 0x35,
        PREFIX_REG_SS = 0x36,
        AAA = 0x37,
        CMP_Eb_Gb = 0x38,
        CMP_Ev_Gv = 0x39,
        CMP_Gb_Eb = 0x3A,
        CMP_Gv_Ev = 0x3B,
        CMP_AL_Ib = 0x3C,
        CMP_eAX_Iv = 0x3D,
        PREFIX_SEG_DS = 0x3E,
        AAS = 0x3F,
        INC_eAX = 0x40,
        INC_eCX = 0x41,
        INC_eDX = 0x42,
        INC_eBX = 0x43,
        INC_eSP = 0x44,
        INC_eBP = 0x45,
        INC_eSI = 0x46,
        INC_eDI = 0x47,
        DEC_eAX = 0x48,
        DEC_eCX = 0x49,
        DEC_eDX = 0x4A,
        DEC_eBX = 0x4B,
        DEC_eSP = 0x4C,
        DEC_eBP = 0x4D,
        DEC_eSI = 0x4E,
        DEC_eDI = 0x4F,
        PUSH_eAX = 0x50,
        PUSH_eCX = 0x51,
        PUSH_eDX = 0x52,
        PUSH_eBX = 0x53,
        PUSH_eSP = 0x54,
        PUSH_eBP = 0x55,
        PUSH_eSI = 0x56,
        PUSH_eDI = 0x57,
        POP_eAX = 0x58,
        POP_eCX = 0x59,
        POP_eDX = 0x5A,
        POP_eBX = 0x5B,
        POP_eSP = 0x5C,
        POP_eBP = 0x5D,
        POP_eSI = 0x5E,
        POP_eDI = 0x5F,
        PUSH_AD = 0x60,
        POP_AD = 0x61,
        BOUND_Gv_Ma = 0x62,
        ARPL_Ew_Gw = 0x63,
        PREFIX_SEG_FS = 0x64,
        PREFIX_SEG_GS = 0x65,
        PREFIX_OPD_SIZE = 0x66,
        PREFIX_ADDR_SIZE = 0x67,
        PUSH_Iv = 0x68,
        IMUL_Gv_Ev_Iv = 0x69,
        PUSH_Ib = 0x6A,
        IMUL_Gv_Ev_Ib = 0x6B,
        INSB = 0x6C,
        INSD = 0x6D,
        OUTSB = 0x6E,
        OUTSD = 0x6F,
        JO_Jb = 0x70,
        JNO_Jb = 0x71,
        JB_Jb = 0x72,
        JNB_Jb = 0x73,
        JZ_Jb = 0x74,
        JNZ_Jb = 0x75,
        JBE_Jb = 0x76,
        JA_Jb = 0x77,
        JS_Jb = 0x78,
        JNS_Jb = 0x79,
        JPE_Jb = 0x7A,
        JPO_Jb = 0x7B,
        JL_Jb = 0x7C,
        JGE_Jb = 0x7D,
        JLE_Jb = 0x7E,
        JG_Jb = 0x7F,
        IMM_GRP1_Eb_Ib = 0x80,
        IMM_GRP1_Ev_Iv = 0x81,
        IMM_GRP1_Eb_Ib_2 = 0x82,
        IMM_GRP1_Ev_Ib = 0x83,
        TEST_Eb_Gb = 0x84,
        TEST_Ev_Gv = 0x85,
        XCHG_Eb_Gb = 0x86,
        XCHG_Ev_Gv = 0x87,
        MOV_Eb_Gb = 0x88,
        MOV_Ev_Gv = 0x89,
        MOV_Gb_Eb = 0x8A,
        MOV_Gv_Ev = 0x8B,
        MOV_Ew_Sw = 0x8C,
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
        CBW = 0x98,
        CWD = 0x99,
        CALL_Ap = 0x9A,
        WAIT = 0x9B,
        PUSHF = 0x9C,
        POPF = 0x9D,
        SAHF = 0x9E,
        LAHF = 0x9F,
        MOV_AL_Ob = 0xA0,
        MOV_eAX_Ov = 0xA1,
        MOV_Ob_AL = 0xA2,
        MOV_Ov_eAX = 0xA3,
        MOVSB_Yb_Xb = 0xA4,
        MOVSW_Yv_Xv = 0xA5,
        CMPSB_Yb_Xb = 0xA6,
        CMPSW_Yv_Xv = 0xA7,
        TEST_AL_Ib = 0xA8,
        TEST_eAX_Iv = 0xA9,
        STOSB_Yb_AL = 0xAA,
        STOSW_Yv_eAX = 0xAB,
        LODSB_AL_Xb = 0xAC,
        LODSW_eAX_Xv = 0xAD,
        SCASB_AL_Yb = 0xAE,
        SCASW_eAX_Yv = 0xAF,
        MOV_AL_Ib = 0xB0,
        MOV_CL_Ib = 0xB1,
        MOV_DL_Ib = 0xB2,
        MOV_BL_Ib = 0xB3,
        MOV_AH_Ib = 0xB4,
        MOV_CH_Ib = 0xB5,
        MOV_DH_Ib = 0xB6,
        MOV_BH_Ib = 0xB7,
        MOV_eAX_Iv = 0xB8,
        MOV_eCX_Iv = 0xB9,
        MOV_eDX_Iv = 0xBA,
        MOV_eBX_Iv = 0xBB,
        MOV_eSP_Iv = 0xBC,
        MOV_eBP_Iv = 0xBD,
        MOV_eSI_Iv = 0xBE,
        MOV_eDI_Iv = 0xBF,
        SHIFT_GRP2_Eb_Ib = 0xC0,
        SHIFT_GRP2_Ev_Ib = 0xC1,
        RET_Iw = 0xC2,
        RET = 0xC3,
        LES_Gv_Mp = 0xC4,
        LDS_Gv_Mp = 0xC5,
        MOV_Eb_Ib = 0xC6,
        MOV_Ev_Iv = 0xC7,
        ENTER_Iw_Ib = 0xC8,
        LEAVE = 0xC9,
        RETF_Iw = 0xCA,
        RETF = 0xCB,
        INT_3 = 0xCC,
        INT_Ib = 0xCD,
        INTO = 0xCE,
        IRET = 0xCF,
        SHIFT_GRP2_Eb_1 = 0xD0,
        SHIFT_GRP2_Ev_1 = 0xD1,
        SHIFT_GRP2_Eb_CL = 0xD2,
        SHIFT_GRP2_Ev_CL = 0xD3,
        AAM_Ib = 0xD4,
        AAD_Ib = 0xD5,
        SALC = 0xD6,
        XLAT = 0xD7,
        X87_ESC_D8 = 0xD8,
        X87_ESC_D9 = 0xD9,
        X87_ESC_DA = 0xDA,
        X87_ESC_DB = 0xDB,
        X87_ESC_DC = 0xDC,
        X87_ESC_DD = 0xDD,
        X87_ESC_DE = 0xDE,
        X87_ESC_DF = 0xDF,
        LOOPNZ_Jb = 0xE0,
        LOOPZ_Jb = 0xE1,
        LOOP_Jb = 0xE2,
        JCXZ_Jb = 0xE3,
        IN_AL_Ib = 0xE4,
        IN_eAX_Ib = 0xE5,
        OUT_Ib_AL = 0xE6,
        OUT_Ib_eAX = 0xE7,
        CALL_Jv = 0xE8,
        JMP_Jv = 0xE9,
        JMP_Ap = 0xEA,
        JMP_Jb = 0xEB,
        IN_AL_DX = 0xEC,
        IN_eAX_DX = 0xED,
        OUT_DX_AL = 0xEE,
        OUT_DX_eAX = 0xEF,
        PREFIX_LOCK = 0xF0,
        ICEBP = 0xF1,
        PREFIX_REPNZ = 0xF2,
        PREFIX_REPZ = 0xF3,
        HLT = 0xF4,
        CMC = 0xF5,
        UNARY_GRP3_Eb = 0xF6,
        UNARY_GRP3_Ev = 0xF7,
        CLC = 0xF8,
        STC = 0xF9,
        CLI = 0xFA,
        STI = 0xFB,
        CLD = 0xFC,
        STD = 0xFD,
        INC_DEC_GRP4_Eb = 0xFE,
        INC_DEC_GRP5_Ev = 0xFF
    };

    /* Length of the instruction at p[0], or an LDZ_ERR_* code.  IA-32 variant:
     * no REX prefixes, and 0x67 selects 16-bit addressing. */
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
        if (!c.has(total)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }
        return total;
    }

private:
    enum prefix_byte : ldz_u8 {
        pfx_none = 0,
        pfx_66,
        pfx_67,
        pfx_lock,
        pfx_rep,
        pfx_seg,
        pfx_opc_2byte
    };

    /* 0x40-0x4F are INC/DEC here, and 0x62/0xC4/0xC5 are BOUND/LES/LDS, so this
     * table is deliberately narrower than the long-mode one. */
    static constexpr ldz_u8 k_prefix_table[256] = {
        /*      0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
        /* 0 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_opc_2byte,
        /* 1 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 2 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_none,
        /* 3 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_none,
        /* 4 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 5 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 6 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_seg,  pfx_seg,  pfx_66,   pfx_67,   pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 7 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 8 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* 9 */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* A */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* B */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* C */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* D */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* E */ pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none,
        /* F */ pfx_lock, pfx_none, pfx_rep,  pfx_rep,  pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none, pfx_none
    };

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
        /* 8 */ tail_none, tail_iz,   tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* 9 */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_far,  tail_none, tail_none, tail_none, tail_none, tail_none,
        /* A */ tail_moffs, tail_moffs, tail_moffs, tail_moffs, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* B */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_iz,   tail_iz,   tail_iz,   tail_iz,   tail_iz,   tail_iz,   tail_iz,   tail_iz,
        /* C */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* D */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none,
        /* E */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_relz, tail_relz, tail_far,  tail_none, tail_none, tail_none, tail_none, tail_none,
        /* F */ tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none, tail_none
    };

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
            switch (k_prefix_table[byte]) {
            case pfx_none:
            case pfx_opc_2byte:
                return 0;
            case pfx_66:
                if (pf.has_66 != 0) {
                    return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);
                }
                pf.has_66 = 1;
                break;
            case pfx_67:
                if (pf.has_67 != 0) {
                    return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);
                }
                pf.has_67 = 1;
                break;
            case pfx_lock:
                if (pf.has_lock != 0) {
                    return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);
                }
                pf.has_lock = 1;
                break;
            case pfx_rep:
                if (pf.has_rep != 0) {
                    return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);
                }
                pf.has_rep = 1;
                break;
            case pfx_seg:
                pf.has_seg = 1;
                break;
            default:
                return LDZ_ERR_CODE(UNKNOWN_ERROR);
            }

            ++pf.length;
            ++first;
        }

        if (first >= LDZ_MAX_INSNS_LEN) {
            return LDZ_ERR_CODE(UNDEFINED_INSTRUCTION);
        }
        return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
    }

    static ldz_size decode(const cursor& c, const prefix_info& pf, const ldz_size first)
    {
        const ldz_u8 opc = c[first];
        const ldz_size mapped = kIa32LengthTable_1byte_opc[opc];

        switch (mapped) {
        case 0x00:
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

        case 0xA1: {
            const ldz_size body = modrm_len(c, first, 1, pf);
            return is_error(body) ? body : body + SIZE_1_BYTE_IMMEDIATE;
        }

        case 0xA4: {
            const ldz_size body = modrm_len(c, first, 1, pf);
            if (is_error(body)) {
                return body;
            }
            return body + tail_width(k_tail_table[opc], SIZE_4_BYTE_IMMEDIATE, pf);
        }

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
            return group_len(c, first, 1, pf, kIa32LengthTable_x87[opc - 0xD8], &modrm_len, &tail_width);

        default:
            return LDZ_ERR_CODE(UNKNOWN_ERROR);
        }
    }

    static ldz_size tail_width(const ldz_u8 kind, const ldz_size default_width, const prefix_info& pf)
    {
        switch (kind) {
        case tail_iz:
            return imm_z_width(pf.has_66 != 0);
        case tail_relz:
            return rel_z_width(pf.has_66 != 0);
        case tail_moffs:
            return moffs_width(pf.has_67 != 0, false);
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

    static ldz_size modrm_len(const cursor& c, const ldz_size first, const ldz_size opc_size,
                          const prefix_info& pf)
    {
        if (!c.has(first + opc_size + 1)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        const ldz_u8 modrm = c[first + opc_size];
        /* 0x67 switches to 16-bit addressing: no SIB byte, and mod=10 carries a
         * disp16 instead of a disp32. */
        const ldz_size mapped = (pf.has_67 != 0 ? kLengthTable_ModRM16 : kLengthTable_ModRM)[modrm];
        if (mapped != 0) {
            return opc_size + mapped - 1;
        }

        if (!c.has(first + opc_size + 2)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        const ldz_u8 sib = c[first + opc_size + 1];
        const ldz_size disp = test_bits123_for(sib, 0b101) ? SIZE_4_BYTE_DISPLACEMENT : 0;
        return opc_size + 2 + disp;
    }

    static ldz_size process_2byte_opc(const cursor& c, const ldz_size first, const prefix_info& pf)
    {
        if (!c.has(first + 2)) {
            return LDZ_ERR_CODE(INSUFFICIENT_BUFFER);
        }

        const ldz_u8 second = c[first + 1];
        const ldz_size mapped = kIa32LengthTable_2byte_opc[second];
        switch (mapped) {
        case 0x00:
            return modrm_len(c, first, 2, pf);

        case 0xA1: /* ModRM plus one immediate byte: 0F A4 / 0F AC (SHLD/SHRD ib) */
            return with_size(modrm_len(c, first, 2, pf), SIZE_1_BYTE_IMMEDIATE);

        case 0x02:
        case 0x03:
        case 0x06:
            /* The map already counts the trailing field; replace its width
             * rather than adding another one. */
            return 2 + tail_width(k_tail_table_2b[second], mapped - 2, pf);

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

    /* IA-32 one-byte opcode-extension groups: C6/C7/F6/F7/FE/FF (sentinel
     * 0xEE in kIa32LengthTable_1byte_opc).  The null return, and with it the
     * caller's UNKNOWN_ERROR branch, is unreachable: the sentinel-domain
     * static_asserts of GroupTableTest (tests/test_group_tables.cpp) pin every
     * 0xEE cell of that map to an opcode listed below.  Do not delete them. */
    static const ldz_u8 (*pick_grp1_table(const ldz_u8 opc))[256]
    {
        switch (opc) {
        case 0xC6: return &kIa32LengthTable_opcode_extension_0xC6;
        case 0xC7: return &kIa32LengthTable_opcode_extension_0xC7;
        case 0xF6: return &kIa32LengthTable_opcode_extension_0xF6;
        case 0xF7: return &kIa32LengthTable_opcode_extension_0xF7;
        case 0xFE: return &kIa32LengthTable_opcode_extension_0xFE;
        case 0xFF: return &kIa32LengthTable_opcode_extension_0xFF;
        default:   return nullptr;
        }
    }

    /* IA-32 members of the 0F group: 0F 00/01/18/71/72/73/AE/B9/BA/C7
     * (sentinel 0xEE in kIa32LengthTable_2byte_opc).  As above, the null
     * return is unreachable only while the sentinel-domain static_asserts of
     * GroupTableTest (tests/test_group_tables.cpp) hold - do not delete them. */
    static const ldz_u8 (*pick_grp2_table(const ldz_u8 second))[256]
    {
        switch (second) {
        case 0x00: return &kIa32LengthTable_0F_00;
        case 0x01: return &kIa32LengthTable_0F_01;
        case 0x18: return &kIa32LengthTable_0F_18;
        case 0x71: return &kIa32LengthTable_0F_71;
        case 0x72: return &kIa32LengthTable_0F_72;
        case 0x73: return &kIa32LengthTable_0F_73;
        case 0xAE: return &kIa32LengthTable_0F_AE;
        case 0xB9: return &kIa32LengthTable_0F_B9;
        case 0xBA: return &kIa32LengthTable_0F_BA;
        case 0xC7: return &kIa32LengthTable_0F_C7;
        default:   return nullptr;
        }
    }

    static ldz_size with_size(const ldz_size body, const ldz_size tail)
    {
        return is_error(body) ? body : body + tail;
    }
};

} // namespace lendiza::detail

#define OPCODE_X86(x) (static_cast<ldz_u8>(lendiza::detail::x86traits::one_byte_opcode::x))

#endif // !LENDIZA_X86_H
