from enum import Enum

from ._pyjion import *
from collections import namedtuple

# Pre stack effect
Pop0 = 0
Pop1 = 1
PopI = 2
VarPop = 4
PopI4 = 8
PopI8 = 16
PopR4 = 32
PopR8 = 64
PopRef = 128

# Post stack effect
Push0 = 0
Push1 = 1
PushI = 2
VarPush = 4
PushI4 = 8
PushI8 = 16
PushR4 = 32
PushR8 = 64
PushRef = 128

# Size
InlineNone = 0
ShortInlineVar = 1
ShortInlineI = 2
ShortInlineR = 3
InlineI = 4
InlineI8 = 5
InlineR = 6
InlineR8 = 7
InlineMethod = 8
InlineSig = 9
InlineBrTarget = 10
InlineVar = 11
InlineType = 12
InlineField = 13
ShortInlineBrTarget = 14
InlineSwitch = 15
InlineString = 16
InlineTok = 17

# Type
IPrimitive = 1
IMacro = 2
IObjModel = 3
IInternal = 4
IPrefix = 5

NEXT = 1
BREAK = 2
CALL = 3
RETURN = 4
BRANCH = 5
COND_BRANCH = 6
THROW = 7
META = 8

MOOT = None

OPDEF = namedtuple("OPDEF", "cee_code name es_effect_pre es_effect_post size type n_bytes first_byte second_byte flow_arg")


# From pycomp.h
class MethodTokens (Enum):
    METHOD_ADD_TOKEN                         = 0x00000000
    METHOD_MULTIPLY_TOKEN                    = 0x00000001
    METHOD_SUBTRACT_TOKEN                    = 0x00000002
    METHOD_DIVIDE_TOKEN                      = 0x00000003
    METHOD_FLOORDIVIDE_TOKEN                 = 0x00000004
    METHOD_POWER_TOKEN                       = 0x00000005
    METHOD_MODULO_TOKEN                      = 0x00000006
    METHOD_SUBSCR_TOKEN                      = 0x00000007
    METHOD_STOREMAP_TOKEN                    = 0x00000008
    METHOD_RICHCMP_TOKEN                     = 0x00000009
    METHOD_CONTAINS_TOKEN                    = 0x0000000A
    METHOD_NOTCONTAINS_TOKEN                 = 0x0000000B
    METHOD_STORESUBSCR_TOKEN                 = 0x0000000C
    METHOD_DELETESUBSCR_TOKEN                = 0x0000000D
    METHOD_NEWFUNCTION_TOKEN                 = 0x0000000E
    METHOD_GETITER_TOKEN                     = 0x0000000F
    METHOD_DECREF_TOKEN                      = 0x00000010
    METHOD_GETBUILDCLASS_TOKEN               = 0x00000011
    METHOD_LOADNAME_TOKEN                    = 0x00000012
    METHOD_STORENAME_TOKEN                   = 0x00000013
    METHOD_UNPACK_SEQUENCE_TOKEN             = 0x00000014
    METHOD_UNPACK_SEQUENCEEX_TOKEN           = 0x00000015
    METHOD_DELETENAME_TOKEN                  = 0x00000016
    METHOD_PYCELL_SET_TOKEN                  = 0x00000017
    METHOD_SET_CLOSURE                       = 0x00000018
    METHOD_BUILD_SLICE                       = 0x00000019
    METHOD_UNARY_POSITIVE                    = 0x0000001A
    METHOD_UNARY_NEGATIVE                    = 0x0000001B
    METHOD_UNARY_NOT                         = 0x0000001C
    METHOD_UNARY_INVERT                      = 0x0000001D
    METHOD_MATRIX_MULTIPLY_TOKEN             = 0x0000001E
    METHOD_BINARY_LSHIFT_TOKEN               = 0x0000001F
    METHOD_BINARY_RSHIFT_TOKEN               = 0x00000020
    METHOD_BINARY_AND_TOKEN                  = 0x00000021
    METHOD_BINARY_XOR_TOKEN                  = 0x00000022
    METHOD_BINARY_OR_TOKEN                   = 0x00000023
    METHOD_LIST_APPEND_TOKEN                 = 0x00000024
    METHOD_SET_ADD_TOKEN                     = 0x00000025
    METHOD_INPLACE_POWER_TOKEN               = 0x00000026
    METHOD_INPLACE_MULTIPLY_TOKEN            = 0x00000027
    METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN     = 0x00000028
    METHOD_INPLACE_TRUE_DIVIDE_TOKEN         = 0x00000029
    METHOD_INPLACE_FLOOR_DIVIDE_TOKEN        = 0x0000002A
    METHOD_INPLACE_MODULO_TOKEN              = 0x0000002B
    METHOD_INPLACE_ADD_TOKEN                 = 0x0000002C
    METHOD_INPLACE_SUBTRACT_TOKEN            = 0x0000002D
    METHOD_INPLACE_LSHIFT_TOKEN              = 0x0000002E
    METHOD_INPLACE_RSHIFT_TOKEN              = 0x0000002F
    METHOD_INPLACE_AND_TOKEN                 = 0x00000030
    METHOD_INPLACE_XOR_TOKEN                 = 0x00000031
    METHOD_INPLACE_OR_TOKEN                  = 0x00000032
    METHOD_MAP_ADD_TOKEN                     = 0x00000033
    METHOD_PRINT_EXPR_TOKEN                  = 0x00000034
    METHOD_LOAD_CLASSDEREF_TOKEN             = 0x00000035
    METHOD_PREPARE_EXCEPTION                 = 0x00000036
    METHOD_DO_RAISE                          = 0x00000037
    METHOD_EH_TRACE                          = 0x00000038
    METHOD_COMPARE_EXCEPTIONS                = 0x00000039
    METHOD_UNBOUND_LOCAL                     = 0x0000003A
    METHOD_DEBUG_TRACE                       = 0x0000003B
    METHOD_DEBUG_DUMP_FRAME                  = 0x0000003E
    METHOD_UNWIND_EH                         = 0x0000003F
    METHOD_PY_PUSHFRAME                      = 0x00000041
    METHOD_PY_POPFRAME                       = 0x00000042
    METHOD_PY_IMPORTNAME                     = 0x00000043

    METHOD_PY_IMPORTFROM                     = 0x00000045
    METHOD_PY_IMPORTSTAR                     = 0x00000046
    METHOD_IS                                = 0x00000049
    METHOD_ISNOT                             = 0x0000004A
    METHOD_IS_BOOL                           = 0x0000004B
    METHOD_ISNOT_BOOL                        = 0x0000004C
    METHOD_GETITER_OPTIMIZED_TOKEN           = 0x0000004D
    METHOD_COMPARE_EXCEPTIONS_INT            = 0x0000004E

    METHOD_UNARY_NOT_INT                     = 0x00000051
    METHOD_FLOAT_FROM_DOUBLE                 = 0x00000053
    METHOD_BOOL_FROM_LONG                    = 0x00000054
    METHOD_PYERR_SETSTRING                   = 0x00000055
    METHOD_BOX_TAGGED_PTR                    = 0x00000056

    METHOD_EQUALS_INT_TOKEN                  = 0x00000065
    METHOD_LESS_THAN_INT_TOKEN               = 0x00000066
    METHOD_LESS_THAN_EQUALS_INT_TOKEN        = 0x00000067
    METHOD_NOT_EQUALS_INT_TOKEN              = 0x00000068
    METHOD_GREATER_THAN_INT_TOKEN            = 0x00000069
    METHOD_GREATER_THAN_EQUALS_INT_TOKEN     = 0x0000006A
    METHOD_PERIODIC_WORK                     = 0x0000006B

    METHOD_EXTENDLIST_TOKEN                  = 0x0000006C
    METHOD_LISTTOTUPLE_TOKEN                 = 0x0000006D
    METHOD_SETUPDATE_TOKEN                   = 0x0000006E
    METHOD_DICTUPDATE_TOKEN                  = 0x0000006F
    METHOD_UNBOX_LONG_TAGGED                 = 0x00000070

    METHOD_INT_TO_FLOAT                      = 0x00000072

    METHOD_STOREMAP_NO_DECREF_TOKEN          = 0x00000073
    METHOD_FORMAT_VALUE                      = 0x00000074
    METHOD_FORMAT_OBJECT                     = 0x00000075
    METHOD_BUILD_DICT_FROM_TUPLES            = 0x00000076
    METHOD_DICT_MERGE                        = 0x00000077
    METHOD_SETUP_ANNOTATIONS                 = 0x00000078

    METHOD_CALL_0_TOKEN        = 0x00010000
    METHOD_CALL_1_TOKEN        = 0x00010001
    METHOD_CALL_2_TOKEN        = 0x00010002
    METHOD_CALL_3_TOKEN        = 0x00010003
    METHOD_CALL_4_TOKEN        = 0x00010004
    METHOD_METHCALL_0_TOKEN    = 0x00011000
    METHOD_METHCALL_1_TOKEN    = 0x00011001
    METHOD_METHCALL_2_TOKEN    = 0x00011002
    METHOD_METHCALL_3_TOKEN    = 0x00011003
    METHOD_METHCALL_4_TOKEN    = 0x00011004
    METHOD_METHCALLN_TOKEN     = 0x00011005

    METHOD_CALL_ARGS             = 0x0001000A
    METHOD_CALL_KWARGS           = 0x0001000B
    METHOD_PYUNICODE_JOINARRAY   = 0x0002000C
    METHOD_CALLN_TOKEN           = 0x000101FF
    METHOD_KWCALLN_TOKEN         = 0x000103FF
    METHOD_LOAD_METHOD           = 0x00010400
    METHOD_PYTUPLE_NEW           = 0x00020000
    METHOD_PYLIST_NEW            = 0x00020001
    METHOD_PYDICT_NEWPRESIZED    = 0x00020002
    METHOD_PYSET_NEW             = 0x00020003
    METHOD_PYSET_ADD             = 0x00020004
    METHOD_PYOBJECT_ISTRUE       = 0x00020005
    METHOD_PYITER_NEXT           = 0x00020006
    METHOD_PYCELL_GET            = 0x00020007
    METHOD_PYERR_RESTORE         = 0x00020008
    METHOD_PYOBJECT_STR          = 0x00020009
    METHOD_PYOBJECT_REPR         = 0x0002000A
    METHOD_PYOBJECT_ASCII        = 0x0002000B

    METHOD_LOADGLOBAL_TOKEN      = 0x00030000
    METHOD_LOADATTR_TOKEN        = 0x00030001
    METHOD_STOREATTR_TOKEN       = 0x00030002
    METHOD_DELETEATTR_TOKEN      = 0x00030003
    METHOD_STOREGLOBAL_TOKEN     = 0x00030004
    METHOD_DELETEGLOBAL_TOKEN    = 0x00030005
    METHOD_LOAD_ASSERTION_ERROR  = 0x00030006

    METHOD_FLOAT_POWER_TOKEN    = 0x00050000
    METHOD_FLOAT_FLOOR_TOKEN    = 0x00050001
    METHOD_FLOAT_MODULUS_TOKEN  = 0x00050002

    SIG_ITERNEXT_TOKEN            = 0x00040000
    SIG_ITERNEXT_OPTIMIZED_TOKEN  = 0x00040001


# Copy + Paste these from opcode.def and wrap the CEE codes in quotes.

opcodes = [
OPDEF("CEE_NOP",                        "nop",              Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x00,    NEXT),
OPDEF("CEE_BREAK",                      "break",            Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x01,    BREAK),
OPDEF("CEE_LDARG_0",                    "ldarg.0",          Pop0,               Push1,       InlineNone,         IMacro,      1,  0xFF,    0x02,    NEXT),
OPDEF("CEE_LDARG_1",                    "ldarg.1",          Pop0,               Push1,       InlineNone,         IMacro,      1,  0xFF,    0x03,    NEXT),
OPDEF("CEE_LDARG_2",                    "ldarg.2",          Pop0,               Push1,       InlineNone,         IMacro,      1,  0xFF,    0x04,    NEXT),
OPDEF("CEE_LDARG_3",                    "ldarg.3",          Pop0,               Push1,       InlineNone,         IMacro,      1,  0xFF,    0x05,    NEXT),
OPDEF("CEE_LDLOC_0",                    "ldloc.0",          Pop0,               Push1,       InlineNone,         IMacro,      1,  0xFF,    0x06,    NEXT),
OPDEF("CEE_LDLOC_1",                    "ldloc.1",          Pop0,               Push1,       InlineNone,         IMacro,      1,  0xFF,    0x07,    NEXT),
OPDEF("CEE_LDLOC_2",                    "ldloc.2",          Pop0,               Push1,       InlineNone,         IMacro,      1,  0xFF,    0x08,    NEXT),
OPDEF("CEE_LDLOC_3",                    "ldloc.3",          Pop0,               Push1,       InlineNone,         IMacro,      1,  0xFF,    0x09,    NEXT),
OPDEF("CEE_STLOC_0",                    "stloc.0",          Pop1,               Push0,       InlineNone,         IMacro,      1,  0xFF,    0x0A,    NEXT),
OPDEF("CEE_STLOC_1",                    "stloc.1",          Pop1,               Push0,       InlineNone,         IMacro,      1,  0xFF,    0x0B,    NEXT),
OPDEF("CEE_STLOC_2",                    "stloc.2",          Pop1,               Push0,       InlineNone,         IMacro,      1,  0xFF,    0x0C,    NEXT),
OPDEF("CEE_STLOC_3",                    "stloc.3",          Pop1,               Push0,       InlineNone,         IMacro,      1,  0xFF,    0x0D,    NEXT),
OPDEF("CEE_LDARG_S",                    "ldarg.s",          Pop0,               Push1,       ShortInlineVar,     IMacro,      1,  0xFF,    0x0E,    NEXT),
OPDEF("CEE_LDARGA_S",                   "ldarga.s",         Pop0,               PushI,       ShortInlineVar,     IMacro,      1,  0xFF,    0x0F,    NEXT),
OPDEF("CEE_STARG_S",                    "starg.s",          Pop1,               Push0,       ShortInlineVar,     IMacro,      1,  0xFF,    0x10,    NEXT),
OPDEF("CEE_LDLOC_S",                    "ldloc.s",          Pop0,               Push1,       ShortInlineVar,     IMacro,      1,  0xFF,    0x11,    NEXT),
OPDEF("CEE_LDLOCA_S",                   "ldloca.s",         Pop0,               PushI,       ShortInlineVar,     IMacro,      1,  0xFF,    0x12,    NEXT),
OPDEF("CEE_STLOC_S",                    "stloc.s",          Pop1,               Push0,       ShortInlineVar,     IMacro,      1,  0xFF,    0x13,    NEXT),
OPDEF("CEE_LDNULL",                     "ldnull",           Pop0,               PushRef,     InlineNone,         IPrimitive,  1,  0xFF,    0x14,    NEXT),
OPDEF("CEE_LDC_I4_M1",                  "ldc.i4.m1",        Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x15,    NEXT),
OPDEF("CEE_LDC_I4_0",                   "ldc.i4.0",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x16,    NEXT),
OPDEF("CEE_LDC_I4_1",                   "ldc.i4.1",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x17,    NEXT),
OPDEF("CEE_LDC_I4_2",                   "ldc.i4.2",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x18,    NEXT),
OPDEF("CEE_LDC_I4_3",                   "ldc.i4.3",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x19,    NEXT),
OPDEF("CEE_LDC_I4_4",                   "ldc.i4.4",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x1A,    NEXT),
OPDEF("CEE_LDC_I4_5",                   "ldc.i4.5",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x1B,    NEXT),
OPDEF("CEE_LDC_I4_6",                   "ldc.i4.6",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x1C,    NEXT),
OPDEF("CEE_LDC_I4_7",                   "ldc.i4.7",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x1D,    NEXT),
OPDEF("CEE_LDC_I4_8",                   "ldc.i4.8",         Pop0,               PushI,       InlineNone,         IMacro,      1,  0xFF,    0x1E,    NEXT),
OPDEF("CEE_LDC_I4_S",                   "ldc.i4.s",         Pop0,               PushI,       ShortInlineI,       IMacro,      1,  0xFF,    0x1F,    NEXT),
OPDEF("CEE_LDC_I4",                     "ldc.i4",           Pop0,               PushI,       InlineI,            IPrimitive,  1,  0xFF,    0x20,    NEXT),
OPDEF("CEE_LDC_I8",                     "ldc.i8",           Pop0,               PushI8,      InlineI8,           IPrimitive,  1,  0xFF,    0x21,    NEXT),
OPDEF("CEE_LDC_R4",                     "ldc.r4",           Pop0,               PushR4,      ShortInlineR,       IPrimitive,  1,  0xFF,    0x22,    NEXT),
OPDEF("CEE_LDC_R8",                     "ldc.r8",           Pop0,               PushR8,      InlineR,            IPrimitive,  1,  0xFF,    0x23,    NEXT),
OPDEF("CEE_UNUSED49",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x24,    NEXT),
OPDEF("CEE_DUP",                        "dup",              Pop1,               Push1+Push1, InlineNone,         IPrimitive,  1,  0xFF,    0x25,    NEXT),
OPDEF("CEE_POP",                        "pop",              Pop1,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x26,    NEXT),
OPDEF("CEE_JMP",                        "jmp",              Pop0,               Push0,       InlineMethod,       IPrimitive,  1,  0xFF,    0x27,    CALL),
OPDEF("CEE_CALL",                       "call",             VarPop,             VarPush,     InlineMethod,       IPrimitive,  1,  0xFF,    0x28,    CALL),
OPDEF("CEE_CALLI",                      "calli",            VarPop,             VarPush,     InlineSig,          IPrimitive,  1,  0xFF,    0x29,    CALL),
OPDEF("CEE_RET",                        "ret",              VarPop,             Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x2A,    RETURN),
OPDEF("CEE_BR_S",                       "br.s",             Pop0,               Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x2B,    BRANCH),
OPDEF("CEE_BRFALSE_S",                  "brfalse.s",        PopI,               Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x2C,    COND_BRANCH),
OPDEF("CEE_BRTRUE_S",                   "brtrue.s",         PopI,               Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x2D,    COND_BRANCH),
OPDEF("CEE_BEQ_S",                      "beq.s",            Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x2E,    COND_BRANCH),
OPDEF("CEE_BGE_S",                      "bge.s",            Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x2F,    COND_BRANCH),
OPDEF("CEE_BGT_S",                      "bgt.s",            Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x30,    COND_BRANCH),
OPDEF("CEE_BLE_S",                      "ble.s",            Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x31,    COND_BRANCH),
OPDEF("CEE_BLT_S",                      "blt.s",            Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x32,    COND_BRANCH),
OPDEF("CEE_BNE_UN_S",                   "bne.un.s",         Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x33,    COND_BRANCH),
OPDEF("CEE_BGE_UN_S",                   "bge.un.s",         Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x34,    COND_BRANCH),
OPDEF("CEE_BGT_UN_S",                   "bgt.un.s",         Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x35,    COND_BRANCH),
OPDEF("CEE_BLE_UN_S",                   "ble.un.s",         Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x36,    COND_BRANCH),
OPDEF("CEE_BLT_UN_S",                   "blt.un.s",         Pop1+Pop1,          Push0,       ShortInlineBrTarget,IMacro,      1,  0xFF,    0x37,    COND_BRANCH),
OPDEF("CEE_BR",                         "br",               Pop0,               Push0,       InlineBrTarget,     IPrimitive,  1,  0xFF,    0x38,    BRANCH),
OPDEF("CEE_BRFALSE",                    "brfalse",          PopI,               Push0,       InlineBrTarget,     IPrimitive,  1,  0xFF,    0x39,    COND_BRANCH),
OPDEF("CEE_BRTRUE",                     "brtrue",           PopI,               Push0,       InlineBrTarget,     IPrimitive,  1,  0xFF,    0x3A,    COND_BRANCH),
OPDEF("CEE_BEQ",                        "beq",              Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x3B,    COND_BRANCH),
OPDEF("CEE_BGE",                        "bge",              Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x3C,    COND_BRANCH),
OPDEF("CEE_BGT",                        "bgt",              Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x3D,    COND_BRANCH),
OPDEF("CEE_BLE",                        "ble",              Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x3E,    COND_BRANCH),
OPDEF("CEE_BLT",                        "blt",              Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x3F,    COND_BRANCH),
OPDEF("CEE_BNE_UN",                     "bne.un",           Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x40,    COND_BRANCH),
OPDEF("CEE_BGE_UN",                     "bge.un",           Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x41,    COND_BRANCH),
OPDEF("CEE_BGT_UN",                     "bgt.un",           Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x42,    COND_BRANCH),
OPDEF("CEE_BLE_UN",                     "ble.un",           Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x43,    COND_BRANCH),
OPDEF("CEE_BLT_UN",                     "blt.un",           Pop1+Pop1,          Push0,       InlineBrTarget,     IMacro,      1,  0xFF,    0x44,    COND_BRANCH),
OPDEF("CEE_SWITCH",                     "switch",           PopI,               Push0,       InlineSwitch,       IPrimitive,  1,  0xFF,    0x45,    COND_BRANCH),
OPDEF("CEE_LDIND_I1",                   "ldind.i1",         PopI,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x46,    NEXT),
OPDEF("CEE_LDIND_U1",                   "ldind.u1",         PopI,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x47,    NEXT),
OPDEF("CEE_LDIND_I2",                   "ldind.i2",         PopI,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x48,    NEXT),
OPDEF("CEE_LDIND_U2",                   "ldind.u2",         PopI,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x49,    NEXT),
OPDEF("CEE_LDIND_I4",                   "ldind.i4",         PopI,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x4A,    NEXT),
OPDEF("CEE_LDIND_U4",                   "ldind.u4",         PopI,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x4B,    NEXT),
OPDEF("CEE_LDIND_I8",                   "ldind.i8",         PopI,               PushI8,      InlineNone,         IPrimitive,  1,  0xFF,    0x4C,    NEXT),
OPDEF("CEE_LDIND_I",                    "ldind.i",          PopI,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x4D,    NEXT),
OPDEF("CEE_LDIND_R4",                   "ldind.r4",         PopI,               PushR4,      InlineNone,         IPrimitive,  1,  0xFF,    0x4E,    NEXT),
OPDEF("CEE_LDIND_R8",                   "ldind.r8",         PopI,               PushR8,      InlineNone,         IPrimitive,  1,  0xFF,    0x4F,    NEXT),
OPDEF("CEE_LDIND_REF",                  "ldind.ref",        PopI,               PushRef,     InlineNone,         IPrimitive,  1,  0xFF,    0x50,    NEXT),
OPDEF("CEE_STIND_REF",                  "stind.ref",        PopI+PopI,          Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x51,    NEXT),
OPDEF("CEE_STIND_I1",                   "stind.i1",         PopI+PopI,          Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x52,    NEXT),
OPDEF("CEE_STIND_I2",                   "stind.i2",         PopI+PopI,          Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x53,    NEXT),
OPDEF("CEE_STIND_I4",                   "stind.i4",         PopI+PopI,          Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x54,    NEXT),
OPDEF("CEE_STIND_I8",                   "stind.i8",         PopI+PopI8,         Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x55,    NEXT),
OPDEF("CEE_STIND_R4",                   "stind.r4",         PopI+PopR4,         Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x56,    NEXT),
OPDEF("CEE_STIND_R8",                   "stind.r8",         PopI+PopR8,         Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x57,    NEXT),
OPDEF("CEE_ADD",                        "add",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x58,    NEXT),
OPDEF("CEE_SUB",                        "sub",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x59,    NEXT),
OPDEF("CEE_MUL",                        "mul",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x5A,    NEXT),
OPDEF("CEE_DIV",                        "div",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x5B,    NEXT),
OPDEF("CEE_DIV_UN",                     "div.un",           Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x5C,    NEXT),
OPDEF("CEE_REM",                        "rem",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x5D,    NEXT),
OPDEF("CEE_REM_UN",                     "rem.un",           Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x5E,    NEXT),
OPDEF("CEE_AND",                        "and",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x5F,    NEXT),
OPDEF("CEE_OR",                         "or",               Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x60,    NEXT),
OPDEF("CEE_XOR",                        "xor",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x61,    NEXT),
OPDEF("CEE_SHL",                        "shl",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x62,    NEXT),
OPDEF("CEE_SHR",                        "shr",              Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x63,    NEXT),
OPDEF("CEE_SHR_UN",                     "shr.un",           Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x64,    NEXT),
OPDEF("CEE_NEG",                        "neg",              Pop1,               Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x65,    NEXT),
OPDEF("CEE_NOT",                        "not",              Pop1,               Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0x66,    NEXT),
OPDEF("CEE_CONV_I1",                    "conv.i1",          Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x67,    NEXT),
OPDEF("CEE_CONV_I2",                    "conv.i2",          Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x68,    NEXT),
OPDEF("CEE_CONV_I4",                    "conv.i4",          Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x69,    NEXT),
OPDEF("CEE_CONV_I8",                    "conv.i8",          Pop1,               PushI8,      InlineNone,         IPrimitive,  1,  0xFF,    0x6A,    NEXT),
OPDEF("CEE_CONV_R4",                    "conv.r4",          Pop1,               PushR4,      InlineNone,         IPrimitive,  1,  0xFF,    0x6B,    NEXT),
OPDEF("CEE_CONV_R8",                    "conv.r8",          Pop1,               PushR8,      InlineNone,         IPrimitive,  1,  0xFF,    0x6C,    NEXT),
OPDEF("CEE_CONV_U4",                    "conv.u4",          Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x6D,    NEXT),
OPDEF("CEE_CONV_U8",                    "conv.u8",          Pop1,               PushI8,      InlineNone,         IPrimitive,  1,  0xFF,    0x6E,    NEXT),
OPDEF("CEE_CALLVIRT",                   "callvirt",         VarPop,             VarPush,     InlineMethod,       IObjModel,   1,  0xFF,    0x6F,    CALL),
OPDEF("CEE_CPOBJ",                      "cpobj",            PopI+PopI,          Push0,       InlineType,         IObjModel,   1,  0xFF,    0x70,    NEXT),
OPDEF("CEE_LDOBJ",                      "ldobj",            PopI,               Push1,       InlineType,         IObjModel,   1,  0xFF,    0x71,    NEXT),
OPDEF("CEE_LDSTR",                      "ldstr",            Pop0,               PushRef,     InlineString,       IObjModel,   1,  0xFF,    0x72,    NEXT),
OPDEF("CEE_NEWOBJ",                     "newobj",           VarPop,             PushRef,     InlineMethod,       IObjModel,   1,  0xFF,    0x73,    CALL),
OPDEF("CEE_CASTCLASS",                  "castclass",        PopRef,             PushRef,     InlineType,         IObjModel,   1,  0xFF,    0x74,    NEXT),
OPDEF("CEE_ISINST",                     "isinst",           PopRef,             PushI,       InlineType,         IObjModel,   1,  0xFF,    0x75,    NEXT),
OPDEF("CEE_CONV_R_UN",                  "conv.r.un",        Pop1,               PushR8,      InlineNone,         IPrimitive,  1,  0xFF,    0x76,    NEXT),
OPDEF("CEE_UNUSED58",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x77,    NEXT),
OPDEF("CEE_UNUSED1",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0x78,    NEXT),
OPDEF("CEE_UNBOX",                      "unbox",            PopRef,             PushI,       InlineType,         IPrimitive,  1,  0xFF,    0x79,    NEXT),
OPDEF("CEE_THROW",                      "throw",            PopRef,             Push0,       InlineNone,         IObjModel,   1,  0xFF,    0x7A,    THROW),
OPDEF("CEE_LDFLD",                      "ldfld",            PopRef,             Push1,       InlineField,        IObjModel,   1,  0xFF,    0x7B,    NEXT),
OPDEF("CEE_LDFLDA",                     "ldflda",           PopRef,             PushI,       InlineField,        IObjModel,   1,  0xFF,    0x7C,    NEXT),
OPDEF("CEE_STFLD",                      "stfld",            PopRef+Pop1,        Push0,       InlineField,        IObjModel,   1,  0xFF,    0x7D,    NEXT),
OPDEF("CEE_LDSFLD",                     "ldsfld",           Pop0,               Push1,       InlineField,        IObjModel,   1,  0xFF,    0x7E,    NEXT),
OPDEF("CEE_LDSFLDA",                    "ldsflda",          Pop0,               PushI,       InlineField,        IObjModel,   1,  0xFF,    0x7F,    NEXT),
OPDEF("CEE_STSFLD",                     "stsfld",           Pop1,               Push0,       InlineField,        IObjModel,   1,  0xFF,    0x80,    NEXT),
OPDEF("CEE_STOBJ",                      "stobj",            PopI+Pop1,          Push0,       InlineType,         IPrimitive,  1,  0xFF,    0x81,    NEXT),
OPDEF("CEE_CONV_OVF_I1_UN",             "conv.ovf.i1.un",   Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x82,    NEXT),
OPDEF("CEE_CONV_OVF_I2_UN",             "conv.ovf.i2.un",   Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x83,    NEXT),
OPDEF("CEE_CONV_OVF_I4_UN",             "conv.ovf.i4.un",   Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x84,    NEXT),
OPDEF("CEE_CONV_OVF_I8_UN",             "conv.ovf.i8.un",   Pop1,               PushI8,      InlineNone,         IPrimitive,  1,  0xFF,    0x85,    NEXT),
OPDEF("CEE_CONV_OVF_U1_UN",             "conv.ovf.u1.un",   Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x86,    NEXT),
OPDEF("CEE_CONV_OVF_U2_UN",             "conv.ovf.u2.un",   Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x87,    NEXT),
OPDEF("CEE_CONV_OVF_U4_UN",             "conv.ovf.u4.un",   Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x88,    NEXT),
OPDEF("CEE_CONV_OVF_U8_UN",             "conv.ovf.u8.un",   Pop1,               PushI8,      InlineNone,         IPrimitive,  1,  0xFF,    0x89,    NEXT),
OPDEF("CEE_CONV_OVF_I_UN",              "conv.ovf.i.un",    Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x8A,    NEXT),
OPDEF("CEE_CONV_OVF_U_UN",              "conv.ovf.u.un",    Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0x8B,    NEXT),
OPDEF("CEE_BOX",                        "box",              Pop1,               PushRef,     InlineType,         IPrimitive,  1,  0xFF,    0x8C,    NEXT),
OPDEF("CEE_NEWARR",                     "newarr",           PopI,               PushRef,     InlineType,         IObjModel,   1,  0xFF,    0x8D,    NEXT),
OPDEF("CEE_LDLEN",                      "ldlen",            PopRef,             PushI,       InlineNone,         IObjModel,   1,  0xFF,    0x8E,    NEXT),
OPDEF("CEE_LDELEMA",                    "ldelema",          PopRef+PopI,        PushI,       InlineType,         IObjModel,   1,  0xFF,    0x8F,    NEXT),
OPDEF("CEE_LDELEM_I1",                  "ldelem.i1",        PopRef+PopI,        PushI,       InlineNone,         IObjModel,   1,  0xFF,    0x90,    NEXT),
OPDEF("CEE_LDELEM_U1",                  "ldelem.u1",        PopRef+PopI,        PushI,       InlineNone,         IObjModel,   1,  0xFF,    0x91,    NEXT),
OPDEF("CEE_LDELEM_I2",                  "ldelem.i2",        PopRef+PopI,        PushI,       InlineNone,         IObjModel,   1,  0xFF,    0x92,    NEXT),
OPDEF("CEE_LDELEM_U2",                  "ldelem.u2",        PopRef+PopI,        PushI,       InlineNone,         IObjModel,   1,  0xFF,    0x93,    NEXT),
OPDEF("CEE_LDELEM_I4",                  "ldelem.i4",        PopRef+PopI,        PushI,       InlineNone,         IObjModel,   1,  0xFF,    0x94,    NEXT),
OPDEF("CEE_LDELEM_U4",                  "ldelem.u4",        PopRef+PopI,        PushI,       InlineNone,         IObjModel,   1,  0xFF,    0x95,    NEXT),
OPDEF("CEE_LDELEM_I8",                  "ldelem.i8",        PopRef+PopI,        PushI8,      InlineNone,         IObjModel,   1,  0xFF,    0x96,    NEXT),
OPDEF("CEE_LDELEM_I",                   "ldelem.i",         PopRef+PopI,        PushI,       InlineNone,         IObjModel,   1,  0xFF,    0x97,    NEXT),
OPDEF("CEE_LDELEM_R4",                  "ldelem.r4",        PopRef+PopI,        PushR4,      InlineNone,         IObjModel,   1,  0xFF,    0x98,    NEXT),
OPDEF("CEE_LDELEM_R8",                  "ldelem.r8",        PopRef+PopI,        PushR8,      InlineNone,         IObjModel,   1,  0xFF,    0x99,    NEXT),
OPDEF("CEE_LDELEM_REF",                 "ldelem.ref",       PopRef+PopI,        PushRef,     InlineNone,         IObjModel,   1,  0xFF,    0x9A,    NEXT),
OPDEF("CEE_STELEM_I",                   "stelem.i",         PopRef+PopI+PopI,   Push0,       InlineNone,         IObjModel,   1,  0xFF,    0x9B,    NEXT),
OPDEF("CEE_STELEM_I1",                  "stelem.i1",        PopRef+PopI+PopI,   Push0,       InlineNone,         IObjModel,   1,  0xFF,    0x9C,    NEXT),
OPDEF("CEE_STELEM_I2",                  "stelem.i2",        PopRef+PopI+PopI,   Push0,       InlineNone,         IObjModel,   1,  0xFF,    0x9D,    NEXT),
OPDEF("CEE_STELEM_I4",                  "stelem.i4",        PopRef+PopI+PopI,   Push0,       InlineNone,         IObjModel,   1,  0xFF,    0x9E,    NEXT),
OPDEF("CEE_STELEM_I8",                  "stelem.i8",        PopRef+PopI+PopI8,  Push0,       InlineNone,         IObjModel,   1,  0xFF,    0x9F,    NEXT),
OPDEF("CEE_STELEM_R4",                  "stelem.r4",        PopRef+PopI+PopR4,  Push0,       InlineNone,         IObjModel,   1,  0xFF,    0xA0,    NEXT),
OPDEF("CEE_STELEM_R8",                  "stelem.r8",        PopRef+PopI+PopR8,  Push0,       InlineNone,         IObjModel,   1,  0xFF,    0xA1,    NEXT),
OPDEF("CEE_STELEM_REF",                 "stelem.ref",       PopRef+PopI+PopRef, Push0,       InlineNone,         IObjModel,   1,  0xFF,    0xA2,    NEXT),
OPDEF("CEE_UNUSED2",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xA3,    NEXT),
OPDEF("CEE_UNUSED3",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xA4,    NEXT),
OPDEF("CEE_UNUSED4",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xA5,    NEXT),
OPDEF("CEE_UNUSED5",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xA6,    NEXT),
OPDEF("CEE_UNUSED6",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xA7,    NEXT),
OPDEF("CEE_UNUSED7",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xA8,    NEXT),
OPDEF("CEE_UNUSED8",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xA9,    NEXT),
OPDEF("CEE_UNUSED9",                    "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xAA,    NEXT),
OPDEF("CEE_UNUSED10",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xAB,    NEXT),
OPDEF("CEE_UNUSED11",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xAC,    NEXT),
OPDEF("CEE_UNUSED12",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xAD,    NEXT),
OPDEF("CEE_UNUSED13",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xAE,    NEXT),
OPDEF("CEE_UNUSED14",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xAF,    NEXT),
OPDEF("CEE_UNUSED15",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xB0,    NEXT),
OPDEF("CEE_UNUSED16",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xB1,    NEXT),
OPDEF("CEE_UNUSED17",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xB2,    NEXT),
OPDEF("CEE_CONV_OVF_I1",                "conv.ovf.i1",      Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xB3,    NEXT),
OPDEF("CEE_CONV_OVF_U1",                "conv.ovf.u1",      Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xB4,    NEXT),
OPDEF("CEE_CONV_OVF_I2",                "conv.ovf.i2",      Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xB5,    NEXT),
OPDEF("CEE_CONV_OVF_U2",                "conv.ovf.u2",      Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xB6,    NEXT),
OPDEF("CEE_CONV_OVF_I4",                "conv.ovf.i4",      Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xB7,    NEXT),
OPDEF("CEE_CONV_OVF_U4",                "conv.ovf.u4",      Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xB8,    NEXT),
OPDEF("CEE_CONV_OVF_I8",                "conv.ovf.i8",      Pop1,               PushI8,      InlineNone,         IPrimitive,  1,  0xFF,    0xB9,    NEXT),
OPDEF("CEE_CONV_OVF_U8",                "conv.ovf.u8",      Pop1,               PushI8,      InlineNone,         IPrimitive,  1,  0xFF,    0xBA,    NEXT),
OPDEF("CEE_UNUSED50",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xBB,    NEXT),
OPDEF("CEE_UNUSED18",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xBC,    NEXT),
OPDEF("CEE_UNUSED19",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xBD,    NEXT),
OPDEF("CEE_UNUSED20",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xBE,    NEXT),
OPDEF("CEE_UNUSED21",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xBF,    NEXT),
OPDEF("CEE_UNUSED22",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xC0,    NEXT),
OPDEF("CEE_UNUSED23",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xC1,    NEXT),
OPDEF("CEE_REFANYVAL",                  "refanyval",        Pop1,               PushI,       InlineType,         IPrimitive,  1,  0xFF,    0xC2,    NEXT),
OPDEF("CEE_CKFINITE",                   "ckfinite",         Pop1,               PushR8,      InlineNone,         IPrimitive,  1,  0xFF,    0xC3,    NEXT),
OPDEF("CEE_UNUSED24",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xC4,    NEXT),
OPDEF("CEE_UNUSED25",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xC5,    NEXT),
OPDEF("CEE_MKREFANY",                   "mkrefany",         PopI,               Push1,       InlineType,         IPrimitive,  1,  0xFF,    0xC6,    NEXT),
OPDEF("CEE_UNUSED59",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xC7,    NEXT),
OPDEF("CEE_UNUSED60",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xC8,    NEXT),
OPDEF("CEE_UNUSED61",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xC9,    NEXT),
OPDEF("CEE_UNUSED62",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xCA,    NEXT),
OPDEF("CEE_UNUSED63",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xCB,    NEXT),
OPDEF("CEE_UNUSED64",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xCC,    NEXT),
OPDEF("CEE_UNUSED65",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xCD,    NEXT),
OPDEF("CEE_UNUSED66",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xCE,    NEXT),
OPDEF("CEE_UNUSED67",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xCF,    NEXT),
OPDEF("CEE_LDTOKEN",                    "ldtoken",          Pop0,               PushI,       InlineTok,          IPrimitive,  1,  0xFF,    0xD0,    NEXT),
OPDEF("CEE_CONV_U2",                    "conv.u2",          Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xD1,    NEXT),
OPDEF("CEE_CONV_U1",                    "conv.u1",          Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xD2,    NEXT),
OPDEF("CEE_CONV_I",                     "conv.i",           Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xD3,    NEXT),
OPDEF("CEE_CONV_OVF_I",                 "conv.ovf.i",       Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xD4,    NEXT),
OPDEF("CEE_CONV_OVF_U",                 "conv.ovf.u",       Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xD5,    NEXT),
OPDEF("CEE_ADD_OVF",                    "add.ovf",          Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0xD6,    NEXT),
OPDEF("CEE_ADD_OVF_UN",                 "add.ovf.un",       Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0xD7,    NEXT),
OPDEF("CEE_MUL_OVF",                    "mul.ovf",          Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0xD8,    NEXT),
OPDEF("CEE_MUL_OVF_UN",                 "mul.ovf.un",       Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0xD9,    NEXT),
OPDEF("CEE_SUB_OVF",                    "sub.ovf",          Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0xDA,    NEXT),
OPDEF("CEE_SUB_OVF_UN",                 "sub.ovf.un",       Pop1+Pop1,          Push1,       InlineNone,         IPrimitive,  1,  0xFF,    0xDB,    NEXT),
OPDEF("CEE_ENDFINALLY",                 "endfinally",       Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xDC,    RETURN),
OPDEF("CEE_LEAVE",                      "leave",            Pop0,               Push0,       InlineBrTarget,     IPrimitive,  1,  0xFF,    0xDD,    BRANCH),
OPDEF("CEE_LEAVE_S",                    "leave.s",          Pop0,               Push0,       ShortInlineBrTarget,IPrimitive,  1,  0xFF,    0xDE,    BRANCH),
OPDEF("CEE_STIND_I",                    "stind.i",          PopI+PopI,          Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xDF,    NEXT),
OPDEF("CEE_CONV_U",                     "conv.u",           Pop1,               PushI,       InlineNone,         IPrimitive,  1,  0xFF,    0xE0,    NEXT),
OPDEF("CEE_UNUSED26",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE1,    NEXT),
OPDEF("CEE_UNUSED27",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE2,    NEXT),
OPDEF("CEE_UNUSED28",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE3,    NEXT),
OPDEF("CEE_UNUSED29",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE4,    NEXT),
OPDEF("CEE_UNUSED30",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE5,    NEXT),
OPDEF("CEE_UNUSED31",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE6,    NEXT),
OPDEF("CEE_UNUSED32",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE7,    NEXT),
OPDEF("CEE_UNUSED33",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE8,    NEXT),
OPDEF("CEE_UNUSED34",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xE9,    NEXT),
OPDEF("CEE_UNUSED35",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xEA,    NEXT),
OPDEF("CEE_UNUSED36",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xEB,    NEXT),
OPDEF("CEE_UNUSED37",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xEC,    NEXT),
OPDEF("CEE_UNUSED38",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xED,    NEXT),
OPDEF("CEE_UNUSED39",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xEE,    NEXT),
OPDEF("CEE_UNUSED40",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xEF,    NEXT),
OPDEF("CEE_UNUSED41",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xF0,    NEXT),
OPDEF("CEE_UNUSED42",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xF1,    NEXT),
OPDEF("CEE_UNUSED43",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xF2,    NEXT),
OPDEF("CEE_UNUSED44",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xF3,    NEXT),
OPDEF("CEE_UNUSED45",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xF4,    NEXT),
OPDEF("CEE_UNUSED46",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xF5,    NEXT),
OPDEF("CEE_UNUSED47",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xF6,    NEXT),
OPDEF("CEE_UNUSED48",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  1,  0xFF,    0xF7,    NEXT),
OPDEF("CEE_PREFIX7",                    "prefix7",          Pop0,               Push0,       InlineNone,         IInternal,   1,  0xFF,    0xF8,    META),
OPDEF("CEE_PREFIX6",                    "prefix6",          Pop0,               Push0,       InlineNone,         IInternal,   1,  0xFF,    0xF9,    META),
OPDEF("CEE_PREFIX5",                    "prefix5",          Pop0,               Push0,       InlineNone,         IInternal,   1,  0xFF,    0xFA,    META),
OPDEF("CEE_PREFIX4",                    "prefix4",          Pop0,               Push0,       InlineNone,         IInternal,   1,  0xFF,    0xFB,    META),
OPDEF("CEE_PREFIX3",                    "prefix3",          Pop0,               Push0,       InlineNone,         IInternal,   1,  0xFF,    0xFC,    META),
OPDEF("CEE_PREFIX2",                    "prefix2",          Pop0,               Push0,       InlineNone,         IInternal,   1,  0xFF,    0xFD,    META),
OPDEF("CEE_PREFIX1",                    "prefix1",          Pop0,               Push0,       InlineNone,         IInternal,   1,  0xFF,    0xFE,    META),
OPDEF("CEE_PREFIXREF",                  "prefixref",        Pop0,               Push0,       InlineNone,         IInternal,   1,  0xFF,    0xFF,    META),
OPDEF("CEE_ARGLIST",                    "arglist",          Pop0,               PushI,       InlineNone,         IPrimitive,  2,  0xFE,    0x00,    NEXT),
OPDEF("CEE_CEQ",                        "ceq",              Pop1+Pop1,          PushI,       InlineNone,         IPrimitive,  2,  0xFE,    0x01,    NEXT),
OPDEF("CEE_CGT",                        "cgt",              Pop1+Pop1,          PushI,       InlineNone,         IPrimitive,  2,  0xFE,    0x02,    NEXT),
OPDEF("CEE_CGT_UN",                     "cgt.un",           Pop1+Pop1,          PushI,       InlineNone,         IPrimitive,  2,  0xFE,    0x03,    NEXT),
OPDEF("CEE_CLT",                        "clt",              Pop1+Pop1,          PushI,       InlineNone,         IPrimitive,  2,  0xFE,    0x04,    NEXT),
OPDEF("CEE_CLT_UN",                     "clt.un",           Pop1+Pop1,          PushI,       InlineNone,         IPrimitive,  2,  0xFE,    0x05,    NEXT),
OPDEF("CEE_LDFTN",                      "ldftn",            Pop0,               PushI,       InlineMethod,       IPrimitive,  2,  0xFE,    0x06,    NEXT),
OPDEF("CEE_LDVIRTFTN",                  "ldvirtftn",        PopRef,             PushI,       InlineMethod,       IPrimitive,  2,  0xFE,    0x07,    NEXT),
OPDEF("CEE_UNUSED56",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x08,    NEXT),
OPDEF("CEE_LDARG",                      "ldarg",            Pop0,               Push1,       InlineVar,          IPrimitive,  2,  0xFE,    0x09,    NEXT),
OPDEF("CEE_LDARGA",                     "ldarga",           Pop0,               PushI,       InlineVar,          IPrimitive,  2,  0xFE,    0x0A,    NEXT),
OPDEF("CEE_STARG",                      "starg",            Pop1,               Push0,       InlineVar,          IPrimitive,  2,  0xFE,    0x0B,    NEXT),
OPDEF("CEE_LDLOC",                      "ldloc",            Pop0,               Push1,       InlineVar,          IPrimitive,  2,  0xFE,    0x0C,    NEXT),
OPDEF("CEE_LDLOCA",                     "ldloca",           Pop0,               PushI,       InlineVar,          IPrimitive,  2,  0xFE,    0x0D,    NEXT),
OPDEF("CEE_STLOC",                      "stloc",            Pop1,               Push0,       InlineVar,          IPrimitive,  2,  0xFE,    0x0E,    NEXT),
OPDEF("CEE_LOCALLOC",                   "localloc",         PopI,               PushI,       InlineNone,         IPrimitive,  2,  0xFE,    0x0F,    NEXT),
OPDEF("CEE_UNUSED57",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x10,    NEXT),
OPDEF("CEE_ENDFILTER",                  "endfilter",        PopI,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x11,    RETURN),
OPDEF("CEE_UNALIGNED",                  "unaligned.",       Pop0,               Push0,       ShortInlineI,       IPrefix,     2,  0xFE,    0x12,    META),
OPDEF("CEE_VOLATILE",                   "volatile.",        Pop0,               Push0,       InlineNone,         IPrefix,     2,  0xFE,    0x13,    META),
OPDEF("CEE_TAILCALL",                   "tail.",            Pop0,               Push0,       InlineNone,         IPrefix,     2,  0xFE,    0x14,    META),
OPDEF("CEE_INITOBJ",                    "initobj",          PopI,               Push0,       InlineType,         IObjModel,   2,  0xFE,    0x15,    NEXT),
OPDEF("CEE_UNUSED68",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x16,    NEXT),
OPDEF("CEE_CPBLK",                      "cpblk",            PopI+PopI+PopI,     Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x17,    NEXT),
OPDEF("CEE_INITBLK",                    "initblk",          PopI+PopI+PopI,     Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x18,    NEXT),
OPDEF("CEE_UNUSED69",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x19,    NEXT),
OPDEF("CEE_RETHROW",                    "rethrow",          Pop0,               Push0,       InlineNone,         IObjModel,   2,  0xFE,    0x1A,    THROW),
OPDEF("CEE_UNUSED51",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x1B,    NEXT),
OPDEF("CEE_SIZEOF",                     "sizeof",           Pop0,               PushI,       InlineType,         IPrimitive,  2,  0xFE,    0x1C,    NEXT),
OPDEF("CEE_REFANYTYPE",                 "refanytype",       Pop1,               PushI,       InlineNone,         IPrimitive,  2,  0xFE,    0x1D,    NEXT),
OPDEF("CEE_UNUSED52",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x1E,    NEXT),
OPDEF("CEE_UNUSED53",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x1F,    NEXT),
OPDEF("CEE_UNUSED54",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x20,    NEXT),
OPDEF("CEE_UNUSED55",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x21,    NEXT),
OPDEF("CEE_UNUSED70",                   "unused",           Pop0,               Push0,       InlineNone,         IPrimitive,  2,  0xFE,    0x22,    NEXT),
]

opcode_map = {}
for opcode in opcodes:
    if opcode.first_byte == 0xFF:
        # single byte opcode
        opcode_map[opcode.second_byte] = opcode
    else:
        opcode_map[opcode.first_byte + opcode.second_byte] = opcode


def print_il(il):
    i = iter(il)
    try:
        pc = 0
        while True:
            first = next(i)
            if first == 0 and pc == 0:
                raise NotImplementedError(f"CorILMethod_FatFormat not yet supported")

            op = opcode_map[first]
            if op.size == InlineNone:
                print(f"[IL_{pc:04x}] - {op.name:15}")
                pc += 1
                continue
            elif op.size == ShortInlineBrTarget:
                target = int.from_bytes((next(i),), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] SB {op.name:15} -> {target}")
                pc += 2
                continue
            elif op.size == InlineBrTarget:
                target = int.from_bytes((next(i), next(i), next(i), next(i)), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] B {op.name:15} -> {target}")
                pc += 5
                continue
            elif op.size == InlineField:
                field = int.from_bytes((next(i), next(i), next(i), next(i)), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] F {op.name:15} ({field})")
                pc += 5
                continue
            elif op.size == InlineI:
                target = int.from_bytes((next(i), next(i), next(i), next(i)), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] I {op.name:15} ({target})")
                pc += 5
                continue
            elif op.size == InlineI8:
                target = int.from_bytes((next(i), next(i), next(i), next(i), next(i), next(i), next(i), next(i)), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] I8 {op.name:15} ({target})")
                pc += 9
                continue
            elif op.size == InlineMethod:
                target = int.from_bytes((next(i), next(i), next(i), next(i)), byteorder='little', signed=True)
                meth = MethodTokens(target)
                print(f"[IL_{pc:04x}] M {op.name:15} ({target} : {meth})")
                pc += 5
                continue
            elif op.size == InlineSig:
                target = int.from_bytes((next(i), next(i), next(i), next(i)), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] S {op.name:15} ({target})")
                pc += 5
                continue
            elif op.size == InlineString:
                target = int.from_bytes((next(i), next(i), next(i), next(i)), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] M {op.name:15} ({target})")
                pc += 5
                continue
            elif op.size == ShortInlineVar:
                target = int.from_bytes((next(i),), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] SV {op.name:15} ({target})")
                pc += 2
                continue
            elif op.size == InlineVar:
                target = int.from_bytes((next(i), next(i)), byteorder='little', signed=True)
                print(f"[IL_{pc:04x}] V {op.name:15} ({target})")
                pc += 3
                continue
            else:
                raise NotImplementedError(f"Haven't implemented IL Opcode with size {op.size}")

    except StopIteration:
        pass


def dis(f):
    """
    Disassemble a code object into IL
    """
    il = dump_il(f)
    if not il:
        print("No IL for this function, it may not have compiled correctly.")
        return
    print_il(il)


def dis_native(f):
    try:
        import distorm3
    except ImportError:
        raise ModuleNotFoundError("Install distorm3 before disassembling native functions")
    code = dump_native(f)
    iterable = distorm3.DecodeGenerator(0x00000000, bytes(code), distorm3.Decode64Bits)
    for (offset, size, instruction, hexdump) in iterable:
        print("%.8x: %s" % (offset, instruction))
