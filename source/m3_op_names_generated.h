// Auto-generated enum for operation names
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
    M3OP_NAME_0XFC = 179,
    M3OP_NAME_TERMINATION = 202,
    M3OP_NAME_I32_TRUNC_S_SAT_F32 = 192,
    M3OP_NAME_I32_TRUNC_U_SAT_F32 = 193,
    M3OP_NAME_I32_TRUNC_S_SAT_F64 = 194,
    M3OP_NAME_I32_TRUNC_U_SAT_F64 = 195,
    M3OP_NAME_I64_TRUNC_S_SAT_F32 = 196,
    M3OP_NAME_I64_TRUNC_U_SAT_F32 = 197,
    M3OP_NAME_I64_TRUNC_S_SAT_F64 = 198,
    M3OP_NAME_I64_TRUNC_U_SAT_F64 = 199,
    M3OP_NAME_MEMORY_COPY = 200,
    M3OP_NAME_MEMORY_FILL = 201,
};

// Auto-generated array of operation names
static const char * const RODATA_ATTR opNames[] = {
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
    "local_get",
    "local_set",
    "local_tee",
    "global_get",
    "global_set",
    "i32_load",
    "i64_load",
    "f32_load",
    "f64_load",
    "i32_load8_s",
    "i32_load8_u",
    "i32_load16_s",
    "i32_load16_u",
    "i64_load8_s",
    "i64_load8_u",
    "i64_load16_s",
    "i64_load16_u",
    "i64_load32_s",
    "i64_load32_u",
    "i32_store",
    "i64_store",
    "f32_store",
    "f64_store",
    "i32_store8",
    "i32_store16",
    "i64_store8",
    "i64_store16",
    "i64_store32",
    "memory_size",
    "memory_grow",
    "i32_const",
    "i64_const",
    "f32_const",
    "f64_const",
    "i32_eqz",
    "i32_eq",
    "i32_ne",
    "i32_lt_s",
    "i32_lt_u",
    "i32_gt_s",
    "i32_gt_u",
    "i32_le_s",
    "i32_le_u",
    "i32_ge_s",
    "i32_ge_u",
    "i64_eqz",
    "i64_eq",
    "i64_ne",
    "i64_lt_s",
    "i64_lt_u",
    "i64_gt_s",
    "i64_gt_u",
    "i64_le_s",
    "i64_le_u",
    "i64_ge_s",
    "i64_ge_u",
    "f32_eq",
    "f32_ne",
    "f32_lt",
    "f32_gt",
    "f32_le",
    "f32_ge",
    "f64_eq",
    "f64_ne",
    "f64_lt",
    "f64_gt",
    "f64_le",
    "f64_ge",
    "i32_clz",
    "i32_ctz",
    "i32_popcnt",
    "i32_add",
    "i32_sub",
    "i32_mul",
    "i32_div_s",
    "i32_div_u",
    "i32_rem_s",
    "i32_rem_u",
    "i32_and",
    "i32_or",
    "i32_xor",
    "i32_shl",
    "i32_shr_s",
    "i32_shr_u",
    "i32_rotl",
    "i32_rotr",
    "i64_clz",
    "i64_ctz",
    "i64_popcnt",
    "i64_add",
    "i64_sub",
    "i64_mul",
    "i64_div_s",
    "i64_div_u",
    "i64_rem_s",
    "i64_rem_u",
    "i64_and",
    "i64_or",
    "i64_xor",
    "i64_shl",
    "i64_shr_s",
    "i64_shr_u",
    "i64_rotl",
    "i64_rotr",
    "f32_abs",
    "f32_neg",
    "f32_ceil",
    "f32_floor",
    "f32_trunc",
    "f32_nearest",
    "f32_sqrt",
    "f32_add",
    "f32_sub",
    "f32_mul",
    "f32_div",
    "f32_min",
    "f32_max",
    "f32_copysign",
    "f64_abs",
    "f64_neg",
    "f64_ceil",
    "f64_floor",
    "f64_trunc",
    "f64_nearest",
    "f64_sqrt",
    "f64_add",
    "f64_sub",
    "f64_mul",
    "f64_div",
    "f64_min",
    "f64_max",
    "f64_copysign",
    "i32_wrap_i64",
    "i32_trunc_s_f32",
    "i32_trunc_u_f32",
    "i32_trunc_s_f64",
    "i32_trunc_u_f64",
    "i64_extend_s_i32",
    "i64_extend_u_i32",
    "i64_trunc_s_f32",
    "i64_trunc_u_f32",
    "i64_trunc_s_f64",
    "i64_trunc_u_f64",
    "f32_convert_s_i32",
    "f32_convert_u_i32",
    "f32_convert_s_i64",
    "f32_convert_u_i64",
    "f32_demote_f64",
    "f64_convert_s_i32",
    "f64_convert_u_i32",
    "f64_convert_s_i64",
    "f64_convert_u_i64",
    "f64_promote_f32",
    "i32_reinterpret_f32",
    "i64_reinterpret_f64",
    "f32_reinterpret_i32",
    "f64_reinterpret_i64",
    "i32_extend8_s",
    "i32_extend16_s",
    "i64_extend8_s",
    "i64_extend16_s",
    "i64_extend32_s",
    "0xFC",
    "termination",
    "i32_trunc_s_sat_f32",
    "i32_trunc_u_sat_f32",
    "i32_trunc_s_sat_f64",
    "i32_trunc_u_sat_f64",
    "i64_trunc_s_sat_f32",
    "i64_trunc_u_sat_f32",
    "i64_trunc_s_sat_f64",
    "i64_trunc_u_sat_f64",
    "memory_copy",
    "memory_fill",
};

// Auto-generated getter function
#ifdef DEBUG
const char* getOpName(uint8_t id) {
    return opNames[id];
}
#endif
