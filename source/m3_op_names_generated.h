// Auto-generated enum for operation names
#pragma once

#if M3_FUNCTIONS_ENUM
enum M3OpNames {
    M3OP_NAME_UNREACHABLE = 0,
    M3OP_NAME_NOP = 1,
    M3OP_NAME_BLOCK = 2,
    M3OP_NAME_LOOP = 3,
    M3OP_NAME_IF = 4,
    M3OP_NAME_ELSE = 5,
    M3OP_NAME_END = 6,
    M3OP_NAME_BR = 7,
    M3OP_NAME_BR_IF = 8,
    M3OP_NAME_BR_TABLE = 9,
    M3OP_NAME_RETURN = 10,
    M3OP_NAME_CALL = 11,
    M3OP_NAME_CALL_INDIRECT = 12,
    M3OP_NAME_RETURN_CALL = 13,
    M3OP_NAME_RETURN_CALL_INDIRECT = 14,
    M3OP_NAME_DROP = 15,
    M3OP_NAME_SELECT = 16,
    M3OP_NAME_LOCAL_GET = 17,
    M3OP_NAME_LOCAL_SET = 18,
    M3OP_NAME_LOCAL_TEE = 19,
    M3OP_NAME_GLOBAL_GET = 20,
    M3OP_NAME_GLOBAL_SET = 21,
    M3OP_NAME_I32_LOAD = 22,
    M3OP_NAME_I64_LOAD = 23,
    M3OP_NAME_F32_LOAD = 24,
    M3OP_NAME_F64_LOAD = 25,
    M3OP_NAME_I32_LOAD8_S = 26,
    M3OP_NAME_I32_LOAD8_U = 27,
    M3OP_NAME_I32_LOAD16_S = 28,
    M3OP_NAME_I32_LOAD16_U = 29,
    M3OP_NAME_I64_LOAD8_S = 30,
    M3OP_NAME_I64_LOAD8_U = 31,
    M3OP_NAME_I64_LOAD16_S = 32,
    M3OP_NAME_I64_LOAD16_U = 33,
    M3OP_NAME_I64_LOAD32_S = 34,
    M3OP_NAME_I64_LOAD32_U = 35,
    M3OP_NAME_I32_STORE = 36,
    M3OP_NAME_I64_STORE = 37,
    M3OP_NAME_F32_STORE = 38,
    M3OP_NAME_F64_STORE = 39,
    M3OP_NAME_I32_STORE8 = 40,
    M3OP_NAME_I32_STORE16 = 41,
    M3OP_NAME_I64_STORE8 = 42,
    M3OP_NAME_I64_STORE16 = 43,
    M3OP_NAME_I64_STORE32 = 44,
    M3OP_NAME_MEMORY_SIZE = 45,
    M3OP_NAME_MEMORY_GROW = 46,
    M3OP_NAME_I32_CONST = 47,
    M3OP_NAME_I64_CONST = 48,
    M3OP_NAME_F32_CONST = 49,
    M3OP_NAME_F64_CONST = 50,
    M3OP_NAME_I32_EQZ = 51,
    M3OP_NAME_I32_EQ = 52,
    M3OP_NAME_I32_NE = 53,
    M3OP_NAME_I32_LT_S = 54,
    M3OP_NAME_I32_LT_U = 55,
    M3OP_NAME_I32_GT_S = 56,
    M3OP_NAME_I32_GT_U = 57,
    M3OP_NAME_I32_LE_S = 58,
    M3OP_NAME_I32_LE_U = 59,
    M3OP_NAME_I32_GE_S = 60,
    M3OP_NAME_I32_GE_U = 61,
    M3OP_NAME_I64_EQZ = 62,
    M3OP_NAME_I64_EQ = 63,
    M3OP_NAME_I64_NE = 64,
    M3OP_NAME_I64_LT_S = 65,
    M3OP_NAME_I64_LT_U = 66,
    M3OP_NAME_I64_GT_S = 67,
    M3OP_NAME_I64_GT_U = 68,
    M3OP_NAME_I64_LE_S = 69,
    M3OP_NAME_I64_LE_U = 70,
    M3OP_NAME_I64_GE_S = 71,
    M3OP_NAME_I64_GE_U = 72,
    M3OP_NAME_F32_EQ = 73,
    M3OP_NAME_F32_NE = 74,
    M3OP_NAME_F32_LT = 75,
    M3OP_NAME_F32_GT = 76,
    M3OP_NAME_F32_LE = 77,
    M3OP_NAME_F32_GE = 78,
    M3OP_NAME_F64_EQ = 79,
    M3OP_NAME_F64_NE = 80,
    M3OP_NAME_F64_LT = 81,
    M3OP_NAME_F64_GT = 82,
    M3OP_NAME_F64_LE = 83,
    M3OP_NAME_F64_GE = 84,
    M3OP_NAME_I32_CLZ = 85,
    M3OP_NAME_I32_CTZ = 86,
    M3OP_NAME_I32_POPCNT = 87,
    M3OP_NAME_I32_ADD = 88,
    M3OP_NAME_I32_SUB = 89,
    M3OP_NAME_I32_MUL = 90,
    M3OP_NAME_I32_DIV_S = 91,
    M3OP_NAME_I32_DIV_U = 92,
    M3OP_NAME_I32_REM_S = 93,
    M3OP_NAME_I32_REM_U = 94,
    M3OP_NAME_I32_AND = 95,
    M3OP_NAME_I32_OR = 96,
    M3OP_NAME_I32_XOR = 97,
    M3OP_NAME_I32_SHL = 98,
    M3OP_NAME_I32_SHR_S = 99,
    M3OP_NAME_I32_SHR_U = 100,
    M3OP_NAME_I32_ROTL = 101,
    M3OP_NAME_I32_ROTR = 102,
    M3OP_NAME_I64_CLZ = 103,
    M3OP_NAME_I64_CTZ = 104,
    M3OP_NAME_I64_POPCNT = 105,
    M3OP_NAME_I64_ADD = 106,
    M3OP_NAME_I64_SUB = 107,
    M3OP_NAME_I64_MUL = 108,
    M3OP_NAME_I64_DIV_S = 109,
    M3OP_NAME_I64_DIV_U = 110,
    M3OP_NAME_I64_REM_S = 111,
    M3OP_NAME_I64_REM_U = 112,
    M3OP_NAME_I64_AND = 113,
    M3OP_NAME_I64_OR = 114,
    M3OP_NAME_I64_XOR = 115,
    M3OP_NAME_I64_SHL = 116,
    M3OP_NAME_I64_SHR_S = 117,
    M3OP_NAME_I64_SHR_U = 118,
    M3OP_NAME_I64_ROTL = 119,
    M3OP_NAME_I64_ROTR = 120,
    M3OP_NAME_F32_ABS = 121,
    M3OP_NAME_F32_NEG = 122,
    M3OP_NAME_F32_CEIL = 123,
    M3OP_NAME_F32_FLOOR = 124,
    M3OP_NAME_F32_TRUNC = 125,
    M3OP_NAME_F32_NEAREST = 126,
    M3OP_NAME_F32_SQRT = 127,
    M3OP_NAME_F32_ADD = 128,
    M3OP_NAME_F32_SUB = 129,
    M3OP_NAME_F32_MUL = 130,
    M3OP_NAME_F32_DIV = 131,
    M3OP_NAME_F32_MIN = 132,
    M3OP_NAME_F32_MAX = 133,
    M3OP_NAME_F32_COPYSIGN = 134,
    M3OP_NAME_F64_ABS = 135,
    M3OP_NAME_F64_NEG = 136,
    M3OP_NAME_F64_CEIL = 137,
    M3OP_NAME_F64_FLOOR = 138,
    M3OP_NAME_F64_TRUNC = 139,
    M3OP_NAME_F64_NEAREST = 140,
    M3OP_NAME_F64_SQRT = 141,
    M3OP_NAME_F64_ADD = 142,
    M3OP_NAME_F64_SUB = 143,
    M3OP_NAME_F64_MUL = 144,
    M3OP_NAME_F64_DIV = 145,
    M3OP_NAME_F64_MIN = 146,
    M3OP_NAME_F64_MAX = 147,
    M3OP_NAME_F64_COPYSIGN = 148,
    M3OP_NAME_I32_WRAP_I64 = 149,
    M3OP_NAME_I32_TRUNC_S_F32 = 150,
    M3OP_NAME_I32_TRUNC_U_F32 = 151,
    M3OP_NAME_I32_TRUNC_S_F64 = 152,
    M3OP_NAME_I32_TRUNC_U_F64 = 153,
    M3OP_NAME_I64_EXTEND_S_I32 = 154,
    M3OP_NAME_I64_EXTEND_U_I32 = 155,
    M3OP_NAME_I64_TRUNC_S_F32 = 156,
    M3OP_NAME_I64_TRUNC_U_F32 = 157,
    M3OP_NAME_I64_TRUNC_S_F64 = 158,
    M3OP_NAME_I64_TRUNC_U_F64 = 159,
    M3OP_NAME_F32_CONVERT_S_I32 = 160,
    M3OP_NAME_F32_CONVERT_U_I32 = 161,
    M3OP_NAME_F32_CONVERT_S_I64 = 162,
    M3OP_NAME_F32_CONVERT_U_I64 = 163,
    M3OP_NAME_F32_DEMOTE_F64 = 164,
    M3OP_NAME_F64_CONVERT_S_I32 = 165,
    M3OP_NAME_F64_CONVERT_U_I32 = 166,
    M3OP_NAME_F64_CONVERT_S_I64 = 167,
    M3OP_NAME_F64_CONVERT_U_I64 = 168,
    M3OP_NAME_F64_PROMOTE_F32 = 169,
    M3OP_NAME_I32_REINTERPRET_F32 = 170,
    M3OP_NAME_I64_REINTERPRET_F64 = 171,
    M3OP_NAME_F32_REINTERPRET_I32 = 172,
    M3OP_NAME_F64_REINTERPRET_I64 = 173,
    M3OP_NAME_I32_EXTEND8_S = 174,
    M3OP_NAME_I32_EXTEND16_S = 175,
    M3OP_NAME_I64_EXTEND8_S = 176,
    M3OP_NAME_I64_EXTEND16_S = 177,
    M3OP_NAME_I64_EXTEND32_S = 178,
    M3OP_NAME_COMPILE = 179,
    M3OP_NAME_ENTRY = 180,
    M3OP_NAME_UNSUPPORTED = 182,
    M3OP_NAME_CALLRAWFUNCTION = 183,
    M3OP_NAME_GETGLOBAL_S32 = 184,
    M3OP_NAME_GETGLOBAL_S64 = 185,
    M3OP_NAME_CONTINUELOOP = 186,
    M3OP_NAME_CONTINUELOOPIF = 187,
    M3OP_NAME_COPYSLOT_32 = 188,
    M3OP_NAME_PRESERVECOPYSLOT_32 = 189,
    M3OP_NAME_IF_S = 190,
    M3OP_NAME_BRANCHIFPROLOGUE_S = 191,
    M3OP_NAME_COPYSLOT_64 = 192,
    M3OP_NAME_PRESERVECOPYSLOT_64 = 193,
    M3OP_NAME_IF_R = 194,
    M3OP_NAME_BRANCHIFPROLOGUE_R = 195,
    M3OP_NAME_SELECT_I32_RSS = 196,
    M3OP_NAME_SELECT_I32_SRS = 197,
    M3OP_NAME_SELECT_I32_SSR = 198,
    M3OP_NAME_SELECT_I32_SSS = 199,
    M3OP_NAME_SELECT_I64_RSS = 200,
    M3OP_NAME_SELECT_I64_SRS = 201,
    M3OP_NAME_SELECT_I64_SSR = 202,
    M3OP_NAME_SELECT_I64_SSS = 203,
    M3OP_NAME_SELECT_F32_SSS = 204,
    M3OP_NAME_SELECT_F32_SRS = 205,
    M3OP_NAME_SELECT_F32_SSR = 206,
    M3OP_NAME_SELECT_F32_RSS = 207,
    M3OP_NAME_SELECT_F32_RRS = 208,
    M3OP_NAME_SELECT_F32_RSR = 209,
    M3OP_NAME_SELECT_F64_SSS = 210,
    M3OP_NAME_SELECT_F64_SRS = 211,
    M3OP_NAME_SELECT_F64_SSR = 212,
    M3OP_NAME_SELECT_F64_RSS = 213,
    M3OP_NAME_SELECT_F64_RRS = 214,
    M3OP_NAME_SELECT_F64_RSR = 215,
    M3OP_NAME_MEMFILL = 216,
    M3OP_NAME_MEMCOPY = 217,
    M3OP_NAME_SETGLOBAL = 218,
    M3OP_NAME_SETGLOBAL_S32 = 219,
    M3OP_NAME_SETGLOBAL_S64 = 220,
    M3OP_NAME_SETREGISTER = 221,
    M3OP_NAME_SETSLOT = 222,
    M3OP_NAME_PRESERVESETSLOT = 223,
    M3OP_NAME_0XFC = 224,
    M3OP_NAME_I32_TRUNC_S_SAT_F32 = 226,
    M3OP_NAME_I32_TRUNC_U_SAT_F32 = 227,
    M3OP_NAME_I32_TRUNC_S_SAT_F64 = 228,
    M3OP_NAME_I32_TRUNC_U_SAT_F64 = 229,
    M3OP_NAME_I64_TRUNC_S_SAT_F32 = 230,
    M3OP_NAME_I64_TRUNC_U_SAT_F32 = 231,
    M3OP_NAME_I64_TRUNC_S_SAT_F64 = 232,
    M3OP_NAME_I64_TRUNC_U_SAT_F64 = 233,
    M3OP_NAME_MEMORY_COPY = 234,
    M3OP_NAME_MEMORY_FILL = 235,
    M3OP_NAME_TERMINATION = 236,
};

// Auto-generated array of operation names
static cstr_t opNames[] __attribute__((section(".rodata"))) = {
    "unreachable",
    "nop",
    "block",
    "loop",
    "if",
    "else",
    "end",
    "br",
    "br_if",
    "br_table",
    "return",
    "call",
    "call_indirect",
    "return_call",
    "return_call_indirect",
    "drop",
    "select",
    "local.get",
    "local.set",
    "local.tee",
    "global.get",
    "global.set",
    "i32.load",
    "i64.load",
    "f32.load",
    "f64.load",
    "i32.load8_s",
    "i32.load8_u",
    "i32.load16_s",
    "i32.load16_u",
    "i64.load8_s",
    "i64.load8_u",
    "i64.load16_s",
    "i64.load16_u",
    "i64.load32_s",
    "i64.load32_u",
    "i32.store",
    "i64.store",
    "f32.store",
    "f64.store",
    "i32.store8",
    "i32.store16",
    "i64.store8",
    "i64.store16",
    "i64.store32",
    "memory.size",
    "memory.grow",
    "i32.const",
    "i64.const",
    "f32.const",
    "f64.const",
    "i32.eqz",
    "i32.eq",
    "i32.ne",
    "i32.lt_s",
    "i32.lt_u",
    "i32.gt_s",
    "i32.gt_u",
    "i32.le_s",
    "i32.le_u",
    "i32.ge_s",
    "i32.ge_u",
    "i64.eqz",
    "i64.eq",
    "i64.ne",
    "i64.lt_s",
    "i64.lt_u",
    "i64.gt_s",
    "i64.gt_u",
    "i64.le_s",
    "i64.le_u",
    "i64.ge_s",
    "i64.ge_u",
    "f32.eq",
    "f32.ne",
    "f32.lt",
    "f32.gt",
    "f32.le",
    "f32.ge",
    "f64.eq",
    "f64.ne",
    "f64.lt",
    "f64.gt",
    "f64.le",
    "f64.ge",
    "i32.clz",
    "i32.ctz",
    "i32.popcnt",
    "i32.add",
    "i32.sub",
    "i32.mul",
    "i32.div_s",
    "i32.div_u",
    "i32.rem_s",
    "i32.rem_u",
    "i32.and",
    "i32.or",
    "i32.xor",
    "i32.shl",
    "i32.shr_s",
    "i32.shr_u",
    "i32.rotl",
    "i32.rotr",
    "i64.clz",
    "i64.ctz",
    "i64.popcnt",
    "i64.add",
    "i64.sub",
    "i64.mul",
    "i64.div_s",
    "i64.div_u",
    "i64.rem_s",
    "i64.rem_u",
    "i64.and",
    "i64.or",
    "i64.xor",
    "i64.shl",
    "i64.shr_s",
    "i64.shr_u",
    "i64.rotl",
    "i64.rotr",
    "f32.abs",
    "f32.neg",
    "f32.ceil",
    "f32.floor",
    "f32.trunc",
    "f32.nearest",
    "f32.sqrt",
    "f32.add",
    "f32.sub",
    "f32.mul",
    "f32.div",
    "f32.min",
    "f32.max",
    "f32.copysign",
    "f64.abs",
    "f64.neg",
    "f64.ceil",
    "f64.floor",
    "f64.trunc",
    "f64.nearest",
    "f64.sqrt",
    "f64.add",
    "f64.sub",
    "f64.mul",
    "f64.div",
    "f64.min",
    "f64.max",
    "f64.copysign",
    "i32.wrap/i64",
    "i32.trunc_s/f32",
    "i32.trunc_u/f32",
    "i32.trunc_s/f64",
    "i32.trunc_u/f64",
    "i64.extend_s/i32",
    "i64.extend_u/i32",
    "i64.trunc_s/f32",
    "i64.trunc_u/f32",
    "i64.trunc_s/f64",
    "i64.trunc_u/f64",
    "f32.convert_s/i32",
    "f32.convert_u/i32",
    "f32.convert_s/i64",
    "f32.convert_u/i64",
    "f32.demote/f64",
    "f64.convert_s/i32",
    "f64.convert_u/i32",
    "f64.convert_s/i64",
    "f64.convert_u/i64",
    "f64.promote/f32",
    "i32.reinterpret/f32",
    "i64.reinterpret/f64",
    "f32.reinterpret/i32",
    "f64.reinterpret/i64",
    "i32.extend8_s",
    "i32.extend16_s",
    "i64.extend8_s",
    "i64.extend16_s",
    "i64.extend32_s",
    "Compile",
    "Entry",
    "End",
    "Unsupported",
    "CallRawFunction",
    "GetGlobal_s32",
    "GetGlobal_s64",
    "ContinueLoop",
    "ContinueLoopIf",
    "CopySlot_32",
    "PreserveCopySlot_32",
    "If_s",
    "BranchIfPrologue_s",
    "CopySlot_64",
    "PreserveCopySlot_64",
    "If_r",
    "BranchIfPrologue_r",
    "Select_i32_rss",
    "Select_i32_srs",
    "Select_i32_ssr",
    "Select_i32_sss",
    "Select_i64_rss",
    "Select_i64_srs",
    "Select_i64_ssr",
    "Select_i64_sss",
    "Select_f32_sss",
    "Select_f32_srs",
    "Select_f32_ssr",
    "Select_f32_rss",
    "Select_f32_rrs",
    "Select_f32_rsr",
    "Select_f64_sss",
    "Select_f64_srs",
    "Select_f64_ssr",
    "Select_f64_rss",
    "Select_f64_rrs",
    "Select_f64_rsr",
    "MemFill",
    "MemCopy",
    "SetGlobal",
    "SetGlobal_s32",
    "SetGlobal_s64",
    "SetRegister",
    "SetSlot",
    "PreserveSetSlot",
    "0xFC",
    "i32.trunc_s:sat/f32",
    "i32.trunc_u:sat/f32",
    "i32.trunc_s:sat/f64",
    "i32.trunc_u:sat/f64",
    "i64.trunc_s:sat/f32",
    "i64.trunc_u:sat/f32",
    "i64.trunc_s:sat/f64",
    "i64.trunc_u:sat/f64",
    "memory.copy",
    "memory.fill",
    "termination",
};

// Auto-generated getter function
static cstr_t getOpName(uint8_t id) {
    return opNames[id];
}
#endif
