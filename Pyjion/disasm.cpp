/*
* The MIT License (MIT)
*
* Copyright (c) Microsoft Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
*/

#include "disasm.h"

char * opcodename(opcode_t op){
    char out [100];
    switch(op) {
        case CEE_NOP:
            sprintf(out, "%20s Pop0 / Push0", "nop");
            break;

        case CEE_BREAK:
            sprintf(out, "%20s Pop0 / Push0", "break");
            break;

        case CEE_LDARG_0:
            sprintf(out, "%20s Pop0 / Push1", "ldarg.0");
            break;

        case CEE_LDARG_1:
            sprintf(out, "%20s Pop0 / Push1", "ldarg.1");
            break;

        case CEE_LDARG_2:
            sprintf(out, "%20s Pop0 / Push1", "ldarg.2");
            break;

        case CEE_LDARG_3:
            sprintf(out, "%20s Pop0 / Push1", "ldarg.3");
            break;

        case CEE_LDLOC_0:
            sprintf(out, "%20s Pop0 / Push1", "ldloc.0");
            break;

        case CEE_LDLOC_1:
            sprintf(out, "%20s Pop0 / Push1", "ldloc.1");
            break;

        case CEE_LDLOC_2:
            sprintf(out, "%20s Pop0 / Push1", "ldloc.2");
            break;

        case CEE_LDLOC_3:
            sprintf(out, "%20s Pop0 / Push1", "ldloc.3");
            break;

        case CEE_STLOC_0:
            sprintf(out, "%20s Pop1 / Push0", "stloc.0");
            break;

        case CEE_STLOC_1:
            sprintf(out, "%20s Pop1 / Push0", "stloc.1");
            break;

        case CEE_STLOC_2:
            sprintf(out, "%20s Pop1 / Push0", "stloc.2");
            break;

        case CEE_STLOC_3:
            sprintf(out, "%20s Pop1 / Push0", "stloc.3");
            break;

        case CEE_LDARG_S:
            sprintf(out, "%20s Pop0 / Push1", "ldarg.s");
            break;

        case CEE_LDARGA_S:
            sprintf(out, "%20s Pop0 / PushI", "ldarga.s");
            break;

        case CEE_STARG_S:
            sprintf(out, "%20s Pop1 / Push0", "starg.s");
            break;

        case CEE_LDLOC_S:
            sprintf(out, "%20s Pop0 / Push1", "ldloc.s");
            break;

        case CEE_LDLOCA_S:
            sprintf(out, "%20s Pop0 / PushI", "ldloca.s");
            break;

        case CEE_STLOC_S:
            sprintf(out, "%20s Pop1 / Push0", "stloc.s");
            break;

        case CEE_LDNULL:
            sprintf(out, "%20s Pop0 / PushRef", "ldnull");
            break;

        case CEE_LDC_I4_M1:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.m1");
            break;

        case CEE_LDC_I4_0:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.0");
            break;

        case CEE_LDC_I4_1:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.1");
            break;

        case CEE_LDC_I4_2:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.2");
            break;

        case CEE_LDC_I4_3:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.3");
            break;

        case CEE_LDC_I4_4:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.4");
            break;

        case CEE_LDC_I4_5:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.5");
            break;

        case CEE_LDC_I4_6:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.6");
            break;

        case CEE_LDC_I4_7:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.7");
            break;

        case CEE_LDC_I4_8:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.8");
            break;

        case CEE_LDC_I4_S:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4.s");
            break;

        case CEE_LDC_I4:
            sprintf(out, "%20s Pop0 / PushI", "ldc.i4");
            break;

        case CEE_LDC_I8:
            sprintf(out, "%20s Pop0 / PushI8", "ldc.i8");
            break;

        case CEE_LDC_R4:
            sprintf(out, "%20s Pop0 / PushR4", "ldc.r4");
            break;

        case CEE_LDC_R8:
            sprintf(out, "%20s Pop0 / PushR8", "ldc.r8");
            break;

        case CEE_UNUSED49:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_DUP:
            sprintf(out, "%20s Pop1 / Push1+Push1", "dup");
            break;

        case CEE_POP:
            sprintf(out, "%20s Pop1 / Push0", "pop");
            break;

        case CEE_JMP:
            sprintf(out, "%20s Pop0 / Push0", "jmp");
            break;

        case CEE_CALL:
            sprintf(out, "%20s VarPop / VarPush", "call");
            break;

        case CEE_CALLI:
            sprintf(out, "%20s VarPop / VarPush", "calli");
            break;

        case CEE_RET:
            sprintf(out, "%20s VarPop / Push0", "ret");
            break;

        case CEE_BR_S:
            sprintf(out, "%20s Pop0 / Push0", "br.s");
            break;

        case CEE_BRFALSE_S:
            sprintf(out, "%20s PopI / Push0", "brfalse.s");
            break;

        case CEE_BRTRUE_S:
            sprintf(out, "%20s PopI / Push0", "brtrue.s");
            break;

        case CEE_BEQ_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "beq.s");
            break;

        case CEE_BGE_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bge.s");
            break;

        case CEE_BGT_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bgt.s");
            break;

        case CEE_BLE_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "ble.s");
            break;

        case CEE_BLT_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "blt.s");
            break;

        case CEE_BNE_UN_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bne.un.s");
            break;

        case CEE_BGE_UN_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bge.un.s");
            break;

        case CEE_BGT_UN_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bgt.un.s");
            break;

        case CEE_BLE_UN_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "ble.un.s");
            break;

        case CEE_BLT_UN_S:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "blt.un.s");
            break;

        case CEE_BR:
            sprintf(out, "%20s Pop0 / Push0", "br");
            break;

        case CEE_BRFALSE:
            sprintf(out, "%20s PopI / Push0", "brfalse");
            break;

        case CEE_BRTRUE:
            sprintf(out, "%20s PopI / Push0", "brtrue");
            break;

        case CEE_BEQ:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "beq");
            break;

        case CEE_BGE:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bge");
            break;

        case CEE_BGT:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bgt");
            break;

        case CEE_BLE:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "ble");
            break;

        case CEE_BLT:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "blt");
            break;

        case CEE_BNE_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bne.un");
            break;

        case CEE_BGE_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bge.un");
            break;

        case CEE_BGT_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "bgt.un");
            break;

        case CEE_BLE_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "ble.un");
            break;

        case CEE_BLT_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push0", "blt.un");
            break;

        case CEE_SWITCH:
            sprintf(out, "%20s PopI / Push0", "switch");
            break;

        case CEE_LDIND_I1:
            sprintf(out, "%20s PopI / PushI", "ldind.i1");
            break;

        case CEE_LDIND_U1:
            sprintf(out, "%20s PopI / PushI", "ldind.u1");
            break;

        case CEE_LDIND_I2:
            sprintf(out, "%20s PopI / PushI", "ldind.i2");
            break;

        case CEE_LDIND_U2:
            sprintf(out, "%20s PopI / PushI", "ldind.u2");
            break;

        case CEE_LDIND_I4:
            sprintf(out, "%20s PopI / PushI", "ldind.i4");
            break;

        case CEE_LDIND_U4:
            sprintf(out, "%20s PopI / PushI", "ldind.u4");
            break;

        case CEE_LDIND_I8:
            sprintf(out, "%20s PopI / PushI8", "ldind.i8");
            break;

        case CEE_LDIND_I:
            sprintf(out, "%20s PopI / PushI", "ldind.i");
            break;

        case CEE_LDIND_R4:
            sprintf(out, "%20s PopI / PushR4", "ldind.r4");
            break;

        case CEE_LDIND_R8:
            sprintf(out, "%20s PopI / PushR8", "ldind.r8");
            break;

        case CEE_LDIND_REF:
            sprintf(out, "%20s PopI / PushRef", "ldind.ref");
            break;

        case CEE_STIND_REF:
            sprintf(out, "%20s PopI+PopI / Push0", "stind.ref");
            break;

        case CEE_STIND_I1:
            sprintf(out, "%20s PopI+PopI / Push0", "stind.i1");
            break;

        case CEE_STIND_I2:
            sprintf(out, "%20s PopI+PopI / Push0", "stind.i2");
            break;

        case CEE_STIND_I4:
            sprintf(out, "%20s PopI+PopI / Push0", "stind.i4");
            break;

        case CEE_STIND_I8:
            sprintf(out, "%20s PopI+PopI8 / Push0", "stind.i8");
            break;

        case CEE_STIND_R4:
            sprintf(out, "%20s PopI+PopR4 / Push0", "stind.r4");
            break;

        case CEE_STIND_R8:
            sprintf(out, "%20s PopI+PopR8 / Push0", "stind.r8");
            break;

        case CEE_ADD:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "add");
            break;

        case CEE_SUB:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "sub");
            break;

        case CEE_MUL:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "mul");
            break;

        case CEE_DIV:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "div");
            break;

        case CEE_DIV_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "div.un");
            break;

        case CEE_REM:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "rem");
            break;

        case CEE_REM_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "rem.un");
            break;

        case CEE_AND:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "and");
            break;

        case CEE_OR:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "or");
            break;

        case CEE_XOR:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "xor");
            break;

        case CEE_SHL:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "shl");
            break;

        case CEE_SHR:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "shr");
            break;

        case CEE_SHR_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "shr.un");
            break;

        case CEE_NEG:
            sprintf(out, "%20s Pop1 / Push1", "neg");
            break;

        case CEE_NOT:
            sprintf(out, "%20s Pop1 / Push1", "not");
            break;

        case CEE_CONV_I1:
            sprintf(out, "%20s Pop1 / PushI", "conv.i1");
            break;

        case CEE_CONV_I2:
            sprintf(out, "%20s Pop1 / PushI", "conv.i2");
            break;

        case CEE_CONV_I4:
            sprintf(out, "%20s Pop1 / PushI", "conv.i4");
            break;

        case CEE_CONV_I8:
            sprintf(out, "%20s Pop1 / PushI8", "conv.i8");
            break;

        case CEE_CONV_R4:
            sprintf(out, "%20s Pop1 / PushR4", "conv.r4");
            break;

        case CEE_CONV_R8:
            sprintf(out, "%20s Pop1 / PushR8", "conv.r8");
            break;

        case CEE_CONV_U4:
            sprintf(out, "%20s Pop1 / PushI", "conv.u4");
            break;

        case CEE_CONV_U8:
            sprintf(out, "%20s Pop1 / PushI8", "conv.u8");
            break;

        case CEE_CALLVIRT:
            sprintf(out, "%20s VarPop / VarPush", "callvirt");
            break;

        case CEE_CPOBJ:
            sprintf(out, "%20s PopI+PopI / Push0", "cpobj");
            break;

        case CEE_LDOBJ:
            sprintf(out, "%20s PopI / Push1", "ldobj");
            break;

        case CEE_LDSTR:
            sprintf(out, "%20s Pop0 / PushRef", "ldstr");
            break;

        case CEE_NEWOBJ:
            sprintf(out, "%20s VarPop / PushRef", "newobj");
            break;

        case CEE_CASTCLASS:
            sprintf(out, "%20s PopRef / PushRef", "castclass");
            break;

        case CEE_ISINST:
            sprintf(out, "%20s PopRef / PushI", "isinst");
            break;

        case CEE_CONV_R_UN:
            sprintf(out, "%20s Pop1 / PushR8", "conv.r.un");
            break;

        case CEE_UNUSED58:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED1:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNBOX:
            sprintf(out, "%20s PopRef / PushI", "unbox");
            break;

        case CEE_THROW:
            sprintf(out, "%20s PopRef / Push0", "throw");
            break;

        case CEE_LDFLD:
            sprintf(out, "%20s PopRef / Push1", "ldfld");
            break;

        case CEE_LDFLDA:
            sprintf(out, "%20s PopRef / PushI", "ldflda");
            break;

        case CEE_STFLD:
            sprintf(out, "%20s PopRef+Pop1 / Push0", "stfld");
            break;

        case CEE_LDSFLD:
            sprintf(out, "%20s Pop0 / Push1", "ldsfld");
            break;

        case CEE_LDSFLDA:
            sprintf(out, "%20s Pop0 / PushI", "ldsflda");
            break;

        case CEE_STSFLD:
            sprintf(out, "%20s Pop1 / Push0", "stsfld");
            break;

        case CEE_STOBJ:
            sprintf(out, "%20s PopI+Pop1 / Push0", "stobj");
            break;

        case CEE_CONV_OVF_I1_UN:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.i1.un");
            break;

        case CEE_CONV_OVF_I2_UN:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.i2.un");
            break;

        case CEE_CONV_OVF_I4_UN:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.i4.un");
            break;

        case CEE_CONV_OVF_I8_UN:
            sprintf(out, "%20s Pop1 / PushI8", "conv.ovf.i8.un");
            break;

        case CEE_CONV_OVF_U1_UN:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.u1.un");
            break;

        case CEE_CONV_OVF_U2_UN:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.u2.un");
            break;

        case CEE_CONV_OVF_U4_UN:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.u4.un");
            break;

        case CEE_CONV_OVF_U8_UN:
            sprintf(out, "%20s Pop1 / PushI8", "conv.ovf.u8.un");
            break;

        case CEE_CONV_OVF_I_UN:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.i.un");
            break;

        case CEE_CONV_OVF_U_UN:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.u.un");
            break;

        case CEE_BOX:
            sprintf(out, "%20s Pop1 / PushRef", "box");
            break;

        case CEE_NEWARR:
            sprintf(out, "%20s PopI / PushRef", "newarr");
            break;

        case CEE_LDLEN:
            sprintf(out, "%20s PopRef / PushI", "ldlen");
            break;

        case CEE_LDELEMA:
            sprintf(out, "%20s PopRef+PopI / PushI", "ldelema");
            break;

        case CEE_LDELEM_I1:
            sprintf(out, "%20s PopRef+PopI / PushI", "ldelem.i1");
            break;

        case CEE_LDELEM_U1:
            sprintf(out, "%20s PopRef+PopI / PushI", "ldelem.u1");
            break;

        case CEE_LDELEM_I2:
            sprintf(out, "%20s PopRef+PopI / PushI", "ldelem.i2");
            break;

        case CEE_LDELEM_U2:
            sprintf(out, "%20s PopRef+PopI / PushI", "ldelem.u2");
            break;

        case CEE_LDELEM_I4:
            sprintf(out, "%20s PopRef+PopI / PushI", "ldelem.i4");
            break;

        case CEE_LDELEM_U4:
            sprintf(out, "%20s PopRef+PopI / PushI", "ldelem.u4");
            break;

        case CEE_LDELEM_I8:
            sprintf(out, "%20s PopRef+PopI / PushI8", "ldelem.i8");
            break;

        case CEE_LDELEM_I:
            sprintf(out, "%20s PopRef+PopI / PushI", "ldelem.i");
            break;

        case CEE_LDELEM_R4:
            sprintf(out, "%20s PopRef+PopI / PushR4", "ldelem.r4");
            break;

        case CEE_LDELEM_R8:
            sprintf(out, "%20s PopRef+PopI / PushR8", "ldelem.r8");
            break;

        case CEE_LDELEM_REF:
            sprintf(out, "%20s PopRef+PopI / PushRef", "ldelem.ref");
            break;

        case CEE_STELEM_I:
            sprintf(out, "%20s PopRef+PopI+PopI / Push0", "stelem.i");
            break;

        case CEE_STELEM_I1:
            sprintf(out, "%20s PopRef+PopI+PopI / Push0", "stelem.i1");
            break;

        case CEE_STELEM_I2:
            sprintf(out, "%20s PopRef+PopI+PopI / Push0", "stelem.i2");
            break;

        case CEE_STELEM_I4:
            sprintf(out, "%20s PopRef+PopI+PopI / Push0", "stelem.i4");
            break;

        case CEE_STELEM_I8:
            sprintf(out, "%20s PopRef+PopI+PopI8 / Push0", "stelem.i8");
            break;

        case CEE_STELEM_R4:
            sprintf(out, "%20s PopRef+PopI+PopR4 / Push0", "stelem.r4");
            break;

        case CEE_STELEM_R8:
            sprintf(out, "%20s PopRef+PopI+PopR8 / Push0", "stelem.r8");
            break;

        case CEE_STELEM_REF:
            sprintf(out, "%20s PopRef+PopI+PopRef / Push0", "stelem.ref");
            break;

        case CEE_LDELEM:
            sprintf(out, "%20s PopRef+PopI / Push1", "ldelem");
            break;

        case CEE_STELEM:
            sprintf(out, "%20s PopRef+PopI+Pop1 / Push0", "stelem");
            break;

        case CEE_UNBOX_ANY:
            sprintf(out, "%20s PopRef / Push1", "unbox.any");
            break;

        case CEE_UNUSED5:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED6:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED7:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED8:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED9:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED10:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED11:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED12:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED13:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED14:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED15:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED16:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED17:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_CONV_OVF_I1:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.i1");
            break;

        case CEE_CONV_OVF_U1:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.u1");
            break;

        case CEE_CONV_OVF_I2:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.i2");
            break;

        case CEE_CONV_OVF_U2:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.u2");
            break;

        case CEE_CONV_OVF_I4:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.i4");
            break;

        case CEE_CONV_OVF_U4:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.u4");
            break;

        case CEE_CONV_OVF_I8:
            sprintf(out, "%20s Pop1 / PushI8", "conv.ovf.i8");
            break;

        case CEE_CONV_OVF_U8:
            sprintf(out, "%20s Pop1 / PushI8", "conv.ovf.u8");
            break;

        case CEE_UNUSED50:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED18:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED19:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED20:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED21:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED22:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED23:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_REFANYVAL:
            sprintf(out, "%20s Pop1 / PushI", "refanyval");
            break;

        case CEE_CKFINITE:
            sprintf(out, "%20s Pop1 / PushR8", "ckfinite");
            break;

        case CEE_UNUSED24:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED25:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_MKREFANY:
            sprintf(out, "%20s PopI / Push1", "mkrefany");
            break;

        case CEE_UNUSED59:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED60:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED61:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED62:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED63:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED64:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED65:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED66:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED67:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_LDTOKEN:
            sprintf(out, "%20s Pop0 / PushI", "ldtoken");
            break;

        case CEE_CONV_U2:
            sprintf(out, "%20s Pop1 / PushI", "conv.u2");
            break;

        case CEE_CONV_U1:
            sprintf(out, "%20s Pop1 / PushI", "conv.u1");
            break;

        case CEE_CONV_I:
            sprintf(out, "%20s Pop1 / PushI", "conv.i");
            break;

        case CEE_CONV_OVF_I:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.i");
            break;

        case CEE_CONV_OVF_U:
            sprintf(out, "%20s Pop1 / PushI", "conv.ovf.u");
            break;

        case CEE_ADD_OVF:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "add.ovf");
            break;

        case CEE_ADD_OVF_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "add.ovf.un");
            break;

        case CEE_MUL_OVF:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "mul.ovf");
            break;

        case CEE_MUL_OVF_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "mul.ovf.un");
            break;

        case CEE_SUB_OVF:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "sub.ovf");
            break;

        case CEE_SUB_OVF_UN:
            sprintf(out, "%20s Pop1+Pop1 / Push1", "sub.ovf.un");
            break;

        case CEE_ENDFINALLY:
            sprintf(out, "%20s Pop0 / Push0", "endfinally");
            break;

        case CEE_LEAVE:
            sprintf(out, "%20s Pop0 / Push0", "leave");
            break;

        case CEE_LEAVE_S:
            sprintf(out, "%20s Pop0 / Push0", "leave.s");
            break;

        case CEE_STIND_I:
            sprintf(out, "%20s PopI+PopI / Push0", "stind.i");
            break;

        case CEE_CONV_U:
            sprintf(out, "%20s Pop1 / PushI", "conv.u");
            break;

        case CEE_UNUSED26:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED27:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED28:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED29:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED30:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED31:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED32:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED33:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED34:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED35:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED36:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED37:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED38:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED39:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED40:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED41:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED42:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED43:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED44:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED45:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED46:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED47:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED48:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_PREFIX7:
            sprintf(out, "%20s Pop0 / Push0", "prefix7");
            break;

        case CEE_PREFIX6:
            sprintf(out, "%20s Pop0 / Push0", "prefix6");
            break;

        case CEE_PREFIX5:
            sprintf(out, "%20s Pop0 / Push0", "prefix5");
            break;

        case CEE_PREFIX4:
            sprintf(out, "%20s Pop0 / Push0", "prefix4");
            break;

        case CEE_PREFIX3:
            sprintf(out, "%20s Pop0 / Push0", "prefix3");
            break;

        case CEE_PREFIX2:
            sprintf(out, "%20s Pop0 / Push0", "prefix2");
            break;

        case CEE_PREFIX1:
            sprintf(out, "%20s Pop0 / Push0", "prefix1");
            break;

        case CEE_PREFIXREF:
            sprintf(out, "%20s Pop0 / Push0", "prefixref");
            break;

        case CEE_ARGLIST:
            sprintf(out, "%20s Pop0 / PushI", "arglist");
            break;

        case CEE_CEQ:
            sprintf(out, "%20s Pop1+Pop1 / PushI", "ceq");
            break;

        case CEE_CGT:
            sprintf(out, "%20s Pop1+Pop1 / PushI", "cgt");
            break;

        case CEE_CGT_UN:
            sprintf(out, "%20s Pop1+Pop1 / PushI", "cgt.un");
            break;

        case CEE_CLT:
            sprintf(out, "%20s Pop1+Pop1 / PushI", "clt");
            break;

        case CEE_CLT_UN:
            sprintf(out, "%20s Pop1+Pop1 / PushI", "clt.un");
            break;

        case CEE_LDFTN:
            sprintf(out, "%20s Pop0 / PushI", "ldftn");
            break;

        case CEE_LDVIRTFTN:
            sprintf(out, "%20s PopRef / PushI", "ldvirtftn");
            break;

        case CEE_UNUSED56:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_LDARG:
            sprintf(out, "%20s Pop0 / Push1", "ldarg");
            break;

        case CEE_LDARGA:
            sprintf(out, "%20s Pop0 / PushI", "ldarga");
            break;

        case CEE_STARG:
            sprintf(out, "%20s Pop1 / Push0", "starg");
            break;

        case CEE_LDLOC:
            sprintf(out, "%20s Pop0 / Push1", "ldloc");
            break;

        case CEE_LDLOCA:
            sprintf(out, "%20s Pop0 / PushI", "ldloca");
            break;

        case CEE_STLOC:
            sprintf(out, "%20s Pop1 / Push0", "stloc");
            break;

        case CEE_LOCALLOC:
            sprintf(out, "%20s PopI / PushI", "localloc");
            break;

        case CEE_UNUSED57:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_ENDFILTER:
            sprintf(out, "%20s PopI / Push0", "endfilter");
            break;

        case CEE_UNALIGNED:
            sprintf(out, "%20s Pop0 / Push0", "unaligned.");
            break;

        case CEE_VOLATILE:
            sprintf(out, "%20s Pop0 / Push0", "volatile.");
            break;

        case CEE_TAILCALL:
            sprintf(out, "%20s Pop0 / Push0", "tail.");
            break;

        case CEE_INITOBJ:
            sprintf(out, "%20s PopI / Push0", "initobj");
            break;

        case CEE_CONSTRAINED:
            sprintf(out, "%20s Pop0 / Push0", "constrained.");
            break;

        case CEE_CPBLK:
            sprintf(out, "%20s PopI+PopI+PopI / Push0", "cpblk");
            break;

        case CEE_INITBLK:
            sprintf(out, "%20s PopI+PopI+PopI / Push0", "initblk");
            break;

        case CEE_UNUSED69:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_RETHROW:
            sprintf(out, "%20s Pop0 / Push0", "rethrow");
            break;

        case CEE_UNUSED51:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_SIZEOF:
            sprintf(out, "%20s Pop0 / PushI", "sizeof");
            break;

        case CEE_REFANYTYPE:
            sprintf(out, "%20s Pop1 / PushI", "refanytype");
            break;

        case CEE_READONLY:
            sprintf(out, "%20s Pop0 / Push0", "readonly.");
            break;

        case CEE_UNUSED53:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED54:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED55:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

        case CEE_UNUSED70:
            sprintf(out, "%20s Pop0 / Push0", "unused");
            break;

    }
    return out;
};

