#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

enum Opcodes {
    opcode_nop = 0x00, opcode_aconst_null = 0x01, opcode_iconst_m1 = 0x02,
    opcode_iconst_0 = 0x03, opcode_iconst_1 = 0x04, opcode_iconst_2 = 0x05,
    opcode_iconst_3 = 0x06, opcode_iconst_4 = 0x07, opcode_iconst_5 = 0x08,
    opcode_lconst_0 = 0x09, opcode_lconst_1 = 0x0A, opcode_fconst_0 = 0x0B,
    opcode_fconst_1 = 0x0C, opcode_fconst_2 = 0x0D, opcode_dconst_0 = 0x0E,
    opcode_dconst_1 = 0x0F, opcode_bipush = 0x10, opcode_sipush = 0x11,
    opcode_ldc = 0x12, opcode_ldc_w = 0x13, opcode_ldc2_w = 0x14,
    opcode_iload = 0x15, opcode_lload = 0x16, opcode_fload = 0x17,
    opcode_dload = 0x18, opcode_aload = 0x19, opcode_iload_0 = 0x1A,
    opcode_iload_1 = 0x1B, opcode_iload_2 = 0x1C, opcode_iload_3 = 0x1D,
    opcode_lload_0 = 0x1E, opcode_lload_1 = 0x1F, opcode_lload_2 = 0x20,
    opcode_lload_3 = 0x21, opcode_fload_0 = 0x22, opcode_fload_1 = 0x23,
    opcode_fload_2 = 0x24, opcode_fload_3 = 0x25, opcode_dload_0 = 0x26,
    opcode_dload_1 = 0x27, opcode_dload_2 = 0x28, opcode_dload_3 = 0x29,
    opcode_aload_0 = 0x2A, opcode_aload_1 = 0x2B, opcode_aload_2 = 0x2C,
    opcode_aload_3 = 0x2D, opcode_iaload = 0x2E, opcode_laload = 0x2F,
    opcode_faload = 0x30, opcode_daload = 0x31, opcode_aaload = 0x32,
    opcode_baload = 0x33, opcode_caload = 0x34, opcode_saload = 0x35,
    opcode_istore = 0x36, opcode_lstore = 0x37, opcode_fstore = 0x38,
    opcode_dstore = 0x39, opcode_astore = 0x3A, opcode_istore_0 = 0x3B,
    opcode_istore_1 = 0x3C, opcode_istore_2 = 0x3D, opcode_istore_3 = 0x3E,
    opcode_lstore_0 = 0x3F, opcode_lstore_1 = 0x40, opcode_lstore_2 = 0x41,
    opcode_lstore_3 = 0x42, opcode_fstore_0 = 0x43, opcode_fstore_1 = 0x44,
    opcode_fstore_2 = 0x45, opcode_fstore_3 = 0x46, opcode_dstore_0 = 0x47,
    opcode_dstore_1 = 0x48, opcode_dstore_2 = 0x49, opcode_dstore_3 = 0x4A,
    opcode_astore_0 = 0x4B, opcode_astore_1 = 0x4C, opcode_astore_2 = 0x4D,
    opcode_astore_3 = 0x4E, opcode_iastore = 0x4F, opcode_lastore = 0x50,
    opcode_fastore = 0x51, opcode_dastore = 0x52, opcode_aastore = 0x53,
    opcode_bastore = 0x54, opcode_castore = 0x55, opcode_sastore = 0x56,
    opcode_pop = 0x57, opcode_pop2 = 0x58, opcode_dup = 0x59,
    opcode_dup_x1 = 0x5A, opcode_dup_x2 = 0x5B, opcode_dup2 = 0x5C,
    opcode_dup2_x1 = 0x5D, opcode_dup2_x2 = 0x5E, opcode_swap = 0x5F,
    opcode_iadd = 0x60, opcode_ladd = 0x61, opcode_fadd = 0x62,
    opcode_dadd = 0x63, opcode_isub = 0x64, opcode_lsub = 0x65,
    opcode_fsub = 0x66, opcode_dsub = 0x67, opcode_imul = 0x68,
    opcode_lmul = 0x69, opcode_fmul = 0x6A, opcode_dmul = 0x6B,
    opcode_idiv = 0x6C, opcode_ldiv = 0x6D, opcode_fdiv = 0x6E,
    opcode_ddiv = 0x6F, opcode_irem = 0x70, opcode_lrem = 0x71,
    opcode_frem = 0x72, opcode_drem = 0x73, opcode_ineg = 0x74,
    opcode_lneg = 0x75, opcode_fneg = 0x76, opcode_dneg = 0x77,
    opcode_ishl = 0x78, opcode_lshl = 0x79, opcode_ishr = 0x7A,
    opcode_lshr = 0x7B, opcode_iushr = 0x7C, opcode_lushr = 0x7D,
    opcode_iand = 0x7E, opcode_land = 0x7F, opcode_ior = 0x80,
    opcode_lor = 0x81, opcode_ixor = 0x82, opcode_lxor = 0x83,
    opcode_iinc = 0x84, opcode_i2l = 0x85, opcode_i2f = 0x86,
    opcode_i2d = 0x87, opcode_l2i = 0x88, opcode_l2f = 0x89,
    opcode_l2d = 0x8A, opcode_f2i = 0x8B, opcode_f2l = 0x8C,
    opcode_f2d = 0x8D, opcode_d2i = 0x8E, opcode_d2l = 0x8F,
    opcode_d2f = 0x90, opcode_i2b = 0x91, opcode_i2c = 0x92,
    opcode_i2s = 0x93, opcode_lcmp = 0x94, opcode_fcmpl = 0x95,
    opcode_fcmpg = 0x96, opcode_dcmpl = 0x97, opcode_dcmpg = 0x98,
    opcode_ifeq = 0x99, opcode_ifne = 0x9A, opcode_iflt = 0x9B,
    opcode_ifge = 0x9C, opcode_ifgt = 0x9D, opcode_ifle = 0x9E,
    opcode_if_icmpeq = 0x9F, opcode_if_icmpne = 0xA0, opcode_if_icmplt = 0xA1,
    opcode_if_icmpge = 0xA2, opcode_if_icmpgt = 0xA3, opcode_if_icmple = 0xA4,
    opcode_if_acmpeq = 0xA5, opcode_if_acmpne = 0xA6, opcode_goto = 0xA7,
    opcode_jsr = 0xA8, opcode_ret = 0xA9, opcode_tableswitch = 0xAA,
    opcode_lookupswitch = 0xAB, opcode_ireturn = 0xAC, opcode_lreturn = 0xAD,
    opcode_freturn = 0xAE, opcode_dreturn = 0xAF, opcode_areturn = 0xB0,
    opcode_return = 0xB1, opcode_getstatic = 0xB2, opcode_putstatic = 0xB3,
    opcode_getfield = 0xB4, opcode_putfield = 0xB5, opcode_invokevirtual = 0xB6,
    opcode_invokespecial = 0xB7, opcode_invokestatic = 0xB8, opcode_invokeinterface = 0xB9,
    opcode_invokedynamic = 0xBA, opcode_new = 0xBB, opcode_newarray = 0xBC,
    opcode_anewarray = 0xBD, opcode_arraylength = 0xBE, opcode_athrow = 0xBF,
    opcode_checkcast = 0xC0, opcode_instanceof = 0xC1, opcode_monitorenter = 0xC2,
    opcode_monitorexit = 0xC3, opcode_wide = 0xC4, opcode_multianewarray = 0xC5,
    opcode_ifnull = 0xC6, opcode_ifnonnull = 0xC7, opcode_goto_w = 0xC8,
    opcode_jsr_w = 0xC9,

    // Reserved
    opcode_breakpoint = 0xCA, opcode_impdep1 = 0xFE, opcode_impdep2 = 0xFF
};

typedef enum Opcode_newarray_type {
    T_BOOLEAN = 4,
    T_CHAR,
    T_FLOAT,
    T_DOUBLE,
    T_BYTE,
    T_SHORT,
    T_INT,
    T_LONG
} Opcode_newarray_type;

const char* decodeOpcodeNewarrayType(uint8_t type);
const char* getOpcodeMnemonic(uint8_t opcode);

#endif // OPCODES_H
