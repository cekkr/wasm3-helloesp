//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.

#pragma once

#include "m3_math_utils.h"
#include "m3_env.h"
#include "m3_core.h"
#include "m3_compile.h"
#include "m3_info.h"
#include "m3_exec_defs.h"
#include "m3_helpers.h"
#include "m3_op_names_generated.h"
#include "m3_segmented_memory.h"
#include "wasm3_defs.h"
#include <stdint.h>

#if PASSTHROUGH_HELLOESP
#include "wasm3.h"
#endif

// TODO: all these functions could move over to the .c at some point. normally, I'd say screw it,
// but it might prove useful to be able to compile m3_exec alone w/ optimizations while the remaining
// code is at debug O0


// About the naming convention of these operations/macros (_rs, _sr_, _ss, _srs, etc.)
//------------------------------------------------------------------------------------------------------
//   - 'r' means register and 's' means slot
//   - the first letter is the top of the stack
//
//  so, for example, _rs means the first operand (the first thing pushed to the stack) is in a slot
//  and the second operand (the top of the stack) is in a register
//------------------------------------------------------------------------------------------------------

#ifndef M3_COMPILE_OPCODES
//#  error "Opcodes should only be included in one compilation unit"
#endif

#include <limits.h>

// Segmented memory blocks management: https://claude.site/artifacts/1bd111d2-d502-422d-ba33-073490d042e6

d_m3BeginExternC

// Riscrive l'operazione precedente con un nuovo operatore
#define rewrite_op(OP)              *((void**)(_pc-1)) = (void*)(OP)

// Salta un valore immediato nel program counter
#define skip_immediate(TYPE)        (_pc++)

// Legge un valore immediato dal program counter e lo incrementa
#define immediate_ptr(TYPE)             (TYPE*)m3SegmentedMemAccess(_mem, _pc++, sizeof(TYPE))
#define immediate(TYPE)                 *immediate_ptr(TYPE)

// Accede al valore nello slot dello stack usando un offset immediato
#define slot(TYPE)                  *(TYPE*)m3SegmentedMemAccess(_mem, _sp + immediate(i32), sizeof(TYPE))

// Ottiene il puntatore allo slot dello stack usando un offset immediato
#define slot_ptr(TYPE)             (TYPE*)m3SegmentedMemAccess(_mem, _sp + immediate(i32), sizeof(TYPE))

# if d_m3EnableOpProfiling || d_m3EnableOpTracing // originally only d_m3EnableOpProfiling
   # if M3_FUNCTIONS_ENUM
        d_m3RetSig  traceOp   (d_m3OpSig, int i_operation);
    #else 
        d_m3RetSig  traceOp   (d_m3OpSig, cstr_t i_operationName);
    #endif
    #define nextOp()                 M3_MUSTTAIL return traceOp (d_m3OpAllArgs, i_operationName)
# else
#   define nextOp()                 nextOpDirect()
# endif

#define jumpOp(PC)                  jumpOpDirect(PC)

#if d_m3RecordBacktraces
    #define pushBacktraceFrame()            (PushBacktraceFrame (_mem->runtime, _pc - 1))
    #define fillBacktraceFrame(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

    #define newTrap(err)                    return (pushBacktraceFrame (), err)
    #define forwardTrap(err)                return err
#else
    #define pushBacktraceFrame()            do {} while (0)
    #define fillBacktraceFrame(FUNCTION)    do {} while (0)

    #define newTrap(err)                    return backtrace_err(err)
    #define forwardTrap(err)                return backtrace_err(err)
#endif


#if d_m3EnableStrace == 1
    // Flat trace
    #define d_m3TracePrepare
    #define d_m3TracePrint(fmt, ...)            printf(fmt "\n", ##__VA_ARGS__)
#elif d_m3EnableStrace >= 2
    // Structured trace
    #define d_m3TracePrepare                    const IM3Runtime trace_rt = m3MemRuntime(_mem);
    #define d_m3TracePrint(fmt, ...)            printf("%*s" fmt "\n", (trace_rt->callDepth)*2, "", ##__VA_ARGS__)
#else
    #define d_m3TracePrepare
    #define d_m3TracePrint(fmt, ...)
#endif

#if d_m3EnableStrace >= 3 || WASM_TRACE_LOADSTORE
    #define d_m3TraceLoad(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
    #define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#else
    #define d_m3TraceLoad(TYPE,offset,val)
    #define d_m3TraceStore(TYPE,offset,val)
#endif

#if DEBUG
  #define d_nullPointer newTrap (ErrorRuntime (m3Err_nullPointer,   \
                        _mem->runtime, "Null pointer"))

  #define d_outOfBounds newTrap (ErrorRuntime (m3Err_trapOutOfBoundsMemoryAccess,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->total_size, operand))

  #define d_outOfBoundsMemOp(OFFSET, SIZE) newTrap (ErrorRuntime (m3Err_trapOutOfBoundsMemoryAccess,   \
                      _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                      _mem->total_size, OFFSET, SIZE))
#else
  #define d_nullPointer newTrap (m3Err_nullPointer)

  #define d_outOfBounds newTrap (m3Err_trapOutOfBoundsMemoryAccess)

#   define d_outOfBoundsMemOp(OFFSET, SIZE) newTrap (m3Err_trapOutOfBoundsMemoryAccess)

#endif

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#if M3_FUNCTIONS_ENUM
d_m3RetSig  Call  (d_m3OpSig, OP_TRACE_TYPE i_operationName)
#else
d_m3RetSig  Call  (d_m3OpSig, cstr_t i_operationName)
#endif
# else
d_m3RetSig  Call  (d_m3OpSig)
# endif
{
    m3ret_t possible_trap = m3_Yield ();
    if (M3_UNLIKELY(possible_trap)) return possible_trap;

    nextOpDirect();
}

// TODO: OK, this needs some explanation here ;0

#define d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    OP((RES), operand, ((TYPE) REG), ##__VA_ARGS__);    \
    nextOp ();                                          \
    return m3Err_none;                                  \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = slot (TYPE);                        \
    TYPE operand1 = slot (TYPE);                        \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    nextOp ();                                          \
    return m3Err_none;                                  \
}

#define d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    OP((RES), ((TYPE) REG), operand, ##__VA_ARGS__);    \
    nextOp ();                                          \
    return m3Err_none;                                  \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#define d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#define M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#define M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

#define d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#define d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#define d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#define d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#define d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#define d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#define d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#define d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// compare needs to be distinct for fp 'cause the result must be _r0
#define d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#define d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)

///
///
///

//-----------------------

// signed
d_m3CommutativeOp_i (i32, Equal,            ==)     d_m3CommutativeOp_i (i64, Equal,            ==)
d_m3CommutativeOp_i (i32, NotEqual,         !=)     d_m3CommutativeOp_i (i64, NotEqual,         !=)

d_m3Op_i (i32, LessThan,                    < )     d_m3Op_i (i64, LessThan,                    < )
d_m3Op_i (i32, GreaterThan,                 > )     d_m3Op_i (i64, GreaterThan,                 > )
d_m3Op_i (i32, LessThanOrEqual,             <=)     d_m3Op_i (i64, LessThanOrEqual,             <=)
d_m3Op_i (i32, GreaterThanOrEqual,          >=)     d_m3Op_i (i64, GreaterThanOrEqual,          >=)

// unsigned
d_m3Op_i (u32, LessThan,                    < )     d_m3Op_i (u64, LessThan,                    < )
d_m3Op_i (u32, GreaterThan,                 > )     d_m3Op_i (u64, GreaterThan,                 > )
d_m3Op_i (u32, LessThanOrEqual,             <=)     d_m3Op_i (u64, LessThanOrEqual,             <=)
d_m3Op_i (u32, GreaterThanOrEqual,          >=)     d_m3Op_i (u64, GreaterThanOrEqual,          >=)

#if d_m3HasFloat
d_m3CommutativeCmpOp_f (f32, Equal,         ==)     d_m3CommutativeCmpOp_f (f64, Equal,         ==)
d_m3CommutativeCmpOp_f (f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f (f64, NotEqual,      !=)
d_m3CompareOp_f (f32, LessThan,             < )     d_m3CompareOp_f (f64, LessThan,             < )
d_m3CompareOp_f (f32, GreaterThan,          > )     d_m3CompareOp_f (f64, GreaterThan,          > )
d_m3CompareOp_f (f32, LessThanOrEqual,      <=)     d_m3CompareOp_f (f64, LessThanOrEqual,      <=)
d_m3CompareOp_f (f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f (f64, GreaterThanOrEqual,   >=)
#endif

#define OP_ADD_32(A,B) (i32)((u32)(A) + (u32)(B))
#define OP_ADD_64(A,B) (i64)((u64)(A) + (u64)(B))
#define OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#define OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#define OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#define OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, Add,      OP_ADD_32)  d_m3CommutativeOpFunc_i (i64, Add,      OP_ADD_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, Subtract,            OP_SUB_32)  d_m3OpFunc_i (i64, Subtract,            OP_SUB_64)

#define OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#define OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#define OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#define OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, ShiftLeft,       OP_SHL_32)      d_m3OpFunc_i (u64, ShiftLeft,       OP_SHL_64)
d_m3OpFunc_i (i32, ShiftRight,      OP_SHR_32)      d_m3OpFunc_i (i64, ShiftRight,      OP_SHR_64)
d_m3OpFunc_i (u32, ShiftRight,      OP_SHR_32)      d_m3OpFunc_i (u64, ShiftRight,      OP_SHR_64)

d_m3CommutativeOp_i (u32, And,              &)
d_m3CommutativeOp_i (u32, Or,               |)
d_m3CommutativeOp_i (u32, Xor,              ^)

d_m3CommutativeOp_i (u64, And,              &)
d_m3CommutativeOp_i (u64, Or,               |)
d_m3CommutativeOp_i (u64, Xor,              ^)

#if d_m3HasFloat
d_m3CommutativeOp_f (f32, Add,              +)      d_m3CommutativeOp_f (f64, Add,              +)
d_m3CommutativeOp_f (f32, Multiply,         *)      d_m3CommutativeOp_f (f64, Multiply,         *)
d_m3Op_f (f32, Subtract,                    -)      d_m3Op_f (f64, Subtract,                    -)
d_m3Op_f (f32, Divide,                      /)      d_m3Op_f (f64, Divide,                      /)
#endif

d_m3OpFunc_i(u32, Rotl, rotl32)
d_m3OpFunc_i(u32, Rotr, rotr32)
d_m3OpFunc_i(u64, Rotl, rotl64)
d_m3OpFunc_i(u64, Rotr, rotr64)

d_m3OpMacro_i(u32, Divide, OP_DIV_U);
d_m3OpMacro_i(i32, Divide, OP_DIV_S, INT32_MIN);
d_m3OpMacro_i(u64, Divide, OP_DIV_U);
d_m3OpMacro_i(i64, Divide, OP_DIV_S, INT64_MIN);

d_m3OpMacro_i(u32, Remainder, OP_REM_U);
d_m3OpMacro_i(i32, Remainder, OP_REM_S, INT32_MIN);
d_m3OpMacro_i(u64, Remainder, OP_REM_U);
d_m3OpMacro_i(i64, Remainder, OP_REM_S, INT64_MIN);

#if d_m3HasFloat
d_m3OpFunc_f(f32, Min, min_f32);
d_m3OpFunc_f(f32, Max, max_f32);
d_m3OpFunc_f(f64, Min, min_f64);
d_m3OpFunc_f(f64, Max, max_f64);

d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#endif

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#define d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE operand = slot (TYPE);                     \
    OP((RES), operand, ##__VA_ARGS__);              \
    nextOp ();                                      \
}

#define M3_UNARY(RES, X, OP) (RES) = OP(X)
#define d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#define d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)

#if d_m3HasFloat
d_m3UnaryOp_f (f32, Abs,        fabsf);         d_m3UnaryOp_f (f64, Abs,        fabs);
d_m3UnaryOp_f (f32, Ceil,       ceilf);         d_m3UnaryOp_f (f64, Ceil,       ceil);
d_m3UnaryOp_f (f32, Floor,      floorf);        d_m3UnaryOp_f (f64, Floor,      floor);
d_m3UnaryOp_f (f32, Trunc,      truncf);        d_m3UnaryOp_f (f64, Trunc,      trunc);
d_m3UnaryOp_f (f32, Sqrt,       sqrtf);         d_m3UnaryOp_f (f64, Sqrt,       sqrt);
d_m3UnaryOp_f (f32, Nearest,    rintf);         d_m3UnaryOp_f (f64, Nearest,    rint);
d_m3UnaryOp_f (f32, Negate,     -);             d_m3UnaryOp_f (f64, Negate,     -);
#endif

#define OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i (i32, EqualToZero, OP_EQZ)
d_m3UnaryOp_i (i64, EqualToZero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#if (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
    #define OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
    #define OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
    // for 64-bit instructions branchless approach more preferable
    #define OP_CLZ_64(x) (__builtin_clzll((x) | (1LL <<  0)) + OP_EQZ(x))
    #define OP_CTZ_64(x) (__builtin_ctzll((x) | (1LL << 63)) + OP_EQZ(x))
#elif defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
    #define OP_CLZ_32(x) __builtin_clz(x)
    #define OP_CTZ_32(x) __builtin_ctz(x)
    #define OP_CLZ_64(x) __builtin_clzll(x)
    #define OP_CTZ_64(x) __builtin_ctzll(x)
#else
    #define OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
    #define OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
    #define OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
    #define OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#endif

d_m3UnaryOp_i (u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i (u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i (u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i (u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i (u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i (u64, Popcnt, __builtin_popcountll)

#define OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r){
    _r0 = OP_WRAP_I64((i64) _r0);    
    nextOp ();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 operand = slot (i64);
    _r0 = OP_WRAP_I64(operand);
    nextOp ();
}

// Integer sign extension operations
#define OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#define OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#define OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#define OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#define OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i (i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i (i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i (i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i (i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i (i64, Extend32_s, OP_EXTEND32_S_I64)

#define d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * stack = slot_ptr (FROM);                 \
    OP((DEST), (* stack), ##__VA_ARGS__);           \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * stack = slot_ptr (FROM);                 \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (* stack), ##__VA_ARGS__);         \
    nextOp ();                                      \
}

#if d_m3HasFloat
d_m3TruncMacro(_r0, _fp0, i32, Trunc, f32, OP_I32_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, u32, Trunc, f32, OP_U32_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, i32, Trunc, f64, OP_I32_TRUNC_F64)
d_m3TruncMacro(_r0, _fp0, u32, Trunc, f64, OP_U32_TRUNC_F64)

d_m3TruncMacro(_r0, _fp0, i64, Trunc, f32, OP_I64_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, u64, Trunc, f32, OP_U64_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, i64, Trunc, f64, OP_I64_TRUNC_F64)
d_m3TruncMacro(_r0, _fp0, u64, Trunc, f64, OP_U64_TRUNC_F64)

d_m3TruncMacro(_r0, _fp0, i32, TruncSat, f32, OP_I32_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, u32, TruncSat, f32, OP_U32_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, i32, TruncSat, f64, OP_I32_TRUNC_SAT_F64)
d_m3TruncMacro(_r0, _fp0, u32, TruncSat, f64, OP_U32_TRUNC_SAT_F64)

d_m3TruncMacro(_r0, _fp0, i64, TruncSat, f32, OP_I64_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, u64, TruncSat, f32, OP_U64_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, i64, TruncSat, f64, OP_I64_TRUNC_SAT_F64)
d_m3TruncMacro(_r0, _fp0, u64, TruncSat, f64, OP_U64_TRUNC_SAT_F64)
#endif

#define d_m3TypeModifyOp(REG_TO, REG_FROM, TO, NAME, FROM)  \
d_m3Op(TO##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_TO = (TO) ((FROM) REG_FROM);                        \
    nextOp ();                                               \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_TO = (TO) (from);                                   \
    nextOp ();                                               \
}

// Int to int
d_m3TypeModifyOp (_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp (_r0, _r0, i64, Extend, u32);

// Float to float
#if d_m3HasFloat
d_m3TypeModifyOp (_fp0, _fp0, f32, Demote, f64);
d_m3TypeModifyOp (_fp0, _fp0, f64, Promote, f32);
#endif

#define d_m3TypeConvertOp(REG_TO, REG_FROM, TO, NAME, FROM) \
d_m3Op(TO##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_TO = (TO) ((FROM) REG_FROM);                        \
nextOp ();                                               \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    slot (TO) = (TO) ((FROM) REG_FROM);                     \
nextOp ();                                               \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_TO = (TO) (from);                                   \
nextOp ();                                               \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    slot (TO) = (TO) (from);                                \
nextOp ();                                               \
}

// Int to float
#if d_m3HasFloat
d_m3TypeConvertOp (_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp (_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp (_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp (_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp (_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp (_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp (_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp (_fp0, _r0, f32, Convert, u64);
#endif

#define d_m3ReinterpretOp(REG, TO, SRC, FROM)               \
d_m3Op(TO##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    nextOp ();                                               \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = slot (FROM);                                      \
    REG = u.t;                                              \
    nextOp ();                                               \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = (FROM) SRC;                                       \
    slot (TO) = u.t;                                        \
    nextOp ();                                               \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = slot (FROM);                                      \
    slot (TO) = u.t;                                        \
    nextOp ();                                               \
}

#if d_m3HasFloat
d_m3ReinterpretOp (_r0, i32, _fp0, f32)
d_m3ReinterpretOp (_r0, i64, _fp0, f64)
d_m3ReinterpretOp (_fp0, f32, _r0, i32)
d_m3ReinterpretOp (_fp0, f64, _r0, i64)
#endif


d_m3Op  (GetGlobal_s32){
    u32 * global = immediate (u32 *);
    slot (u32) = * global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    nextOp ();
}


d_m3Op  (GetGlobal_s64)
{
    u64 * global = immediate (u64 *);
    slot (u64) = * global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    nextOp ();
}


d_m3Op  (SetGlobal_i32)
{
    u32 * global = immediate (u32 *);
    * global = (u32) _r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    nextOp ();
}


d_m3Op  (SetGlobal_i64)
{
    u64 * global = immediate (u64 *);
    * global = (u64) _r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    nextOp ();
}


d_m3Op (Call)
{
    pc_t callPC                 = immediate (pc_t);
    i32 stackOffset            = immediate (i32);
    IM3Memory memory           = _mem; //  m3MemInfo (_mem);

    pc_t sp = _sp + stackOffset;

    # if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, memory, d_m3OpDefaultArgs, d_m3BaseCstr);
    # else
    m3ret_t r = Call(callPC, sp, memory, d_m3OpDefaultArgs);
    # endif

    // Non serve più refreshare _mem dato che usiamo la memoria segmentata
    
    if (M3_LIKELY(not r))
        nextOp();
    else
    {
        pushBacktraceFrame();
        forwardTrap(r);
    }
}

d_m3Op (CallIndirect)
{
    u32 tableIndex             = slot (u32);
    IM3Module module           = immediate (IM3Module);
    IM3FuncType type           = immediate (IM3FuncType);
    i32 stackOffset            = immediate (i32);
    IM3Memory memory          =_mem; //  m3MemInfo (_mem);

    pc_t sp = _sp + stackOffset;

    m3ret_t r = m3Err_none;

    if (M3_LIKELY(tableIndex < module->table0Size))
    {
        IM3Function function = module->table0[tableIndex];

        if (M3_LIKELY(function))
        {
            if (M3_LIKELY(type == function->funcType))
            {
                if (M3_UNLIKELY(not function->compiled))
                    r = CompileFunction(function);

                if (M3_LIKELY(not r))
                {
                    # if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, memory, d_m3OpDefaultArgs, d_m3BaseCstr);
                    # else
                    r = Call(function->compiled, sp, memory, d_m3OpDefaultArgs);
                    # endif

                    // Non serve più refreshare _mem
                    
                    if (M3_LIKELY(not r)){
                        nextOpDirect();
                    }
                    else
                    {
                        pushBacktraceFrame();
                        forwardTrap(r);
                    }
                }
            }
            else r = m3Err_trapIndirectCallTypeMismatch;
        }
        else r = m3Err_trapTableElementIsNull;
    }
    else r = m3Err_trapTableIndexOutOfRange;

    if (M3_UNLIKELY(r))
        newTrap(r);
    else forwardTrap(r);
}

DEBUG_TYPE WASM_DEBUG_CallRawFunction = WASM_DEBUG_ALL || (WASM_DEBUG && false);
d_m3Op (CallRawFunction)
{
    CALL_WATCHDOG

    d_m3TracePrepare

    if(WASM_DEBUG_CallRawFunction) ESP_LOGI("WASM3", "CallRawFunction: calling raw function at address %p", _pc);

    M3ImportContext ctx;

    M3RawCall call = (M3RawCall) (MEMACCESS(M3RawCall, _mem, (ptr)_pc++)); // *MEMACCESS..
    if(WASM_DEBUG_CallRawFunction) ESP_LOGI("WASM3", "CallRawFunction: call address %p", call);

    ctx.function = immediate (IM3Function);
    ctx.userdata = immediate (void *);
    u64* const sp = ((u64*)_sp);
    IM3Memory memory = _mem;
    IM3Runtime runtime = m3MemRuntime(_mem);

    if(WASM_DEBUG_CallRawFunction) ESP_LOGI("WASM3", "CallRawFunction: ctx.function: %p, ctx.userdata: %p, sp: %p", ctx.function, ctx.userdata, sp);

    // Aggiungi controlli di sicurezza
    if (runtime == NULL) {
        ESP_LOGE("WASM3", "CallRawFunction: no runtime");
        LOG_FLUSH;
        backtrace();
        forwardTrap(m3Err_nullRuntime);  // o un altro codice di errore appropriato
        return;
    }

    void* stack_backup = NULL;
    if (runtime->stack != NULL) {
        stack_backup = runtime->stack;
    }
    
    runtime->stack = sp;
    
    if(WASM_DEBUG_CallRawFunction) ESP_LOGI("WASM3", "CallRawFunction: ok, i'm calling it");
    m3ret_t possible_trap = call(runtime, &ctx, sp, memory);    
    //m3ret_t possible_trap = (m3ret_t)m3ApiOffsetToPtr(_possible_trap);
    if(WASM_DEBUG_CallRawFunction) ESP_LOGI("WASM3", "CallRawFunction: function called.");

    if (stack_backup != NULL) {
        runtime->stack = stack_backup;
    }

    if (M3_UNLIKELY(possible_trap)) {
        pushBacktraceFrame();
    }
    forwardTrap(possible_trap);
}


d_m3Op  (MemSize)
{
    IM3Memory memory            =_mem; //  m3MemInfo (_mem);

    _r0 = memory->total_size; // istead of memory->numPages

    nextOp ();
}


DEBUG_TYPE WASM_DEBUG_MemGrow = WASM_DEBUG_ALL || (WASM_DEBUG && false);
d_m3Op (MemGrow) //todo: convert it to new memory model
{
    if(WASM_DEBUG_MemGrow) ESP_LOGI("WASM3", "MemGrow called");

    IM3Runtime runtime          = m3MemRuntime(_mem);
    IM3Memory memory            = & runtime->memory;

    i32 numPagesToGrow = _r0;
    if (numPagesToGrow >= 0) {
        _r0 = memory->numPages;

        if(WASM_DEBUG_MemGrow) ESP_LOGI("WASM3", "MemGrow: numPagesToGrow = %d", numPagesToGrow);

        if (M3_LIKELY(numPagesToGrow))
        {
            u32 requiredPages = memory->numPages + numPagesToGrow;

            M3Result r = ResizeMemory (runtime, requiredPages);
            if (r)
                _r0 = -1;
        }
    }
    else
    {
        _r0 = -1;
    }

    nextOp ();
}

// Memory Copy operation
DEBUG_TYPE WASM_DEBUG_MemCopy = WASM_DEBUG_ALL || (WASM_DEBUG && false);
const bool WASM_MemCopy_DisableCheck = true;
d_m3Op (MemCopy)
{
    if(WASM_DEBUG_MemCopy) ESP_LOGI("WASM3", "MemCopy called");

    u32 size = (u32) _r0;
    u32 source = slot (u32);
    u32 destination = slot (u32);
    
    if(!WASM_MemCopy_DisableCheck){
        if (M3_UNLIKELY(!IsValidMemoryAccess(_mem, source, size) ||
                        !IsValidMemoryAccess(_mem, destination, size)))
        {
            d_outOfBoundsMemOp((source > destination) ? source : destination, size);
            return;
        }
    }
    
    M3Result res = m3_memcpy(_mem, destination, source, size);
    if(res != NULL){
        ESP_LOGE("WASM3", "MemCopy: m3_memcpy failed");
        LOG_FLUSH;
        // todo: handle error in operations
    }
    
    nextOp();
}

DEBUG_TYPE WASM_DEBUG_MemFill = WASM_DEBUG_ALL || (WASM_DEBUG && false);
const bool WASM_MemFill_DisableCheck = true;
d_m3Op (MemFill)
{
    if(WASM_DEBUG_MemFill) ESP_LOGI("WASM3", "MemFill called");

    u32 size = (u32) _r0;
    u32 byte = slot (u32);
    u64 destination = slot (u32);
    
    if(!WASM_MemFill_DisableCheck){
        if (M3_UNLIKELY(destination + size > _mem->total_size))
        {
            d_outOfBoundsMemOp (destination, size);
            return;
        }
    }

    M3Result res = m3_memset(_mem, destination, byte, size);
    if(res != NULL){
        ESP_LOGE("WASM3", "MemFill: m3_memset failed");
        LOG_FLUSH;
        // todo: handle error in operations
    }
    
    nextOp();
}


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op  (Compile)
{
    rewrite_op (op_Call);

    IM3Function function        = immediate (IM3Function);

    m3ret_t result = m3Err_none;

    if (M3_UNLIKELY(not function->compiled)) // check to see if function was compiled since this operation was emitted.
        result = CompileFunction (function);

    if (not result)
    {
        // patch up compiled pc and call rewritten op_Call
        * ((void**) --_pc) = (void*) (function->compiled);
        --_pc;
        nextOpDirect ();
    }

    newTrap (result);
}

////////////////////////////////

d_m3Op  (Entry)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = immediate (IM3Function);
    IM3Memory memory = m3MemInfo (_mem);

#define d_m3SkipStackCheck 1
#if d_m3SkipStackCheck
    if (true)
#else
    if (M3_LIKELY ((void *) (_sp + function->maxStackSlots) < _mem->maxStack))
#endif
    {
#if defined(DEBUG)
        function->hits++;
#endif
        u8 * stack = (u8 *) ((m3slot_t *) _sp + function->numRetAndArgSlots);

        m3_memset (_mem, stack, 0x0, function->numLocalBytes);
        stack += function->numLocalBytes;

        if (function->constants)
        {
            m3_memcpy (_mem, stack, function->constants, function->numConstantBytes);
        }

        #if d_m3EnableStrace >= 2
            const char* funName = m3_GetFunctionName(function);
            m3stack_t __sp = MEMPOINT(m3stack_t, _mem, _sp);
            const char* printFunArgs = SPrintFunctionArgList (function, __sp + function->numRetSlots);
            d_m3TracePrint("%s %s {", funName, printFunArgs);

            trace_rt->callDepth++;
        #endif

        m3ret_t r = nextOpImpl ();

#if d_m3EnableStrace >= 2
        trace_rt->callDepth--;

        if (r) {
            d_m3TracePrint("} !trap = %s", (char*)r);
        } else {
            int rettype = GetSingleRetType(function->funcType);
            if (rettype != c_m3Type_none) {
                int size = 256;
                char* str = malloc(size*sizeof(char));
                m3stack_t __sp = MEMPOINT(m3stack_t, _mem, _sp);
                SPrintArg (str, size, __sp, rettype);
                d_m3TracePrint("} = %s", str);
                free(str);
            } else {
                d_m3TracePrint("}");
            }
        }
#endif

        if (M3_UNLIKELY(r)) {
            _mem = memory;
            fillBacktraceFrame ();
        }
        forwardTrap (r);
    }
    else newTrap (m3Err_trapStackOverflow);
}

DEBUG_TYPE WASM_DEBUG_Loop = WASM_DEBUG_ALL || (WASM_DEBUG && false);
d_m3Op  (Loop)
{
    if(WASM_DEBUG_Loop) ESP_LOGI("WASM3", "Loop beginning");

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t r;
    
    IM3Memory memory =_mem; //  m3MemInfo (_mem);
    
    // Manteniamo un contatore delle iterazioni per debug/logging opzionale
    u32 iteration_count = 0;

    do
    {
        #if d_m3EnableStrace >= 3
            d_m3TracePrint("iter {");
            trace_rt->callDepth++;
        #endif
        
        if(WASM_DEBUG_Loop) ESP_LOGI("WASM3", "op_Loop: _pc: %p", _pc);
        r = nextOpImpl ();
        
        #if d_m3EnableStrace >= 3
            trace_rt->callDepth--;
            d_m3TracePrint("}");
        #endif

        // Non abbiamo più bisogno di refreshare _mem poiché non usiamo più
        // un singolo blocco mallocated. Invece, ogni accesso alla memoria
        // passa attraverso m3SegmentedMemAccess che gestisce l'allocazione
        // lazy dei segmenti necessari.
        
        iteration_count++;
        
        // Opzionale: logging periodico per debug
        if (DEBUG_MEMORY && (iteration_count % 1000 == 0)) {
            ESP_LOGD("WASM3", "Loop iteration %lu", iteration_count);
        }
    }
    while (r == _pc);

    if(WASM_DEBUG_Loop) ESP_LOGI("WASM3", "Loop ending.");
    
    forwardTrap (r);
}


d_m3Op  (Branch)
{
    jumpOp (_pc);
}


d_m3Op  (If_r)
{
    i32 condition = (i32) _r0;

    u64 elsePC = immediate (pc_t);

    if (condition)
        nextOp ();
    else
        jumpOp (elsePC);
}


d_m3Op  (If_s)
{
    i32 condition = slot (i32);

    u64 elsePC = immediate (pc_t);

    if (condition)
        nextOp ();
    else
        jumpOp (elsePC);
}


d_m3Op  (BranchTable)
{
    u32 branchIndex = slot (u32);           // branch index is always in a slot
    u32 numTargets  = immediate (u32);

    pc_t * branches = (pc_t *) _pc;

    if (branchIndex > numTargets)
        branchIndex = numTargets; // the default index

    jumpOp (branches [branchIndex]);
}


#define d_m3SetRegisterSetSlot(TYPE, REG) \
d_m3Op  (SetRegister_##TYPE)            \
{                                       \
    REG = slot (TYPE);                  \
    nextOp ();                          \
}                                       \
                                        \
d_m3Op(SetSlot_##TYPE)                 \
{                                       \
    slot (TYPE) = (TYPE) REG;           \
    nextOp ();                           \
}                                       \
                                        \
d_m3Op (PreserveSetSlot_##TYPE)         \
{                                       \
    TYPE * stack     = slot_ptr (TYPE); \
    TYPE * preserve  = slot_ptr (TYPE); \
                                        \
    * preserve = * stack;               \
    * stack = (TYPE) REG;               \
                                        \
    nextOp ();                           \
}

d_m3SetRegisterSetSlot (i32, _r0)
d_m3SetRegisterSetSlot (i64, _r0)
#if d_m3HasFloat
d_m3SetRegisterSetSlot (f32, _fp0)
d_m3SetRegisterSetSlot (f64, _fp0)
#endif

d_m3Op (CopySlot_32){
    u32 * dst = slot_ptr (u32);
    u32 * src = slot_ptr (u32);

    * dst = * src;

    nextOp ();
}


d_m3Op (PreserveCopySlot_32)
{
    u32 * dest      = slot_ptr (u32);
    u32 * src       = slot_ptr (u32);
    u32 * preserve  = slot_ptr (u32);

    * preserve = * dest;
    * dest = * src;

    nextOp ();
}


d_m3Op (CopySlot_64)
{
    u64 * dst = slot_ptr (u64);
    u64 * src = slot_ptr (u64);

    * dst = * src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    nextOp ();
}


d_m3Op (PreserveCopySlot_64)
{
    u64 * dest      = slot_ptr (u64);
    u64 * src       = slot_ptr (u64);
    u64 * preserve  = slot_ptr (u64);

    * preserve = * dest;
    * dest = * src;

    nextOp ();
}


#if d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op  (DumpStack)
{
    u32 opcodeIndex         = immediate (u32);
    u32 stackHeight         = immediate (u32);
    IM3Function function    = immediate (IM3Function);

    cstr_t funcName = (function) ? m3_GetFunctionName(function) : "";

    printf (" %4d ", opcodeIndex);
    printf (" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#if d_m3HasFloat
    printf ("                                    fp0: %" PRIf64 "\n", _fp0);
#endif
    pc_t sp = MEMPOINT(pc_t, _mem, _sp);

    for (u32 i = 0; i < stackHeight; ++i)
    {
        cstr_t kind = "";

        printf ("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, kind, i, (u64) *(sp), (i64) *(sp));

        ++sp;
    }
    printf ("---------------------------------------------------------------------------------------------------------\n");

    nextOpDirect();
}
#endif


#define d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op(Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
nextOp ();                                   \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
nextOp ();                                   \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
nextOp ();                                   \
}


d_m3Select_i (i32, _r0)
d_m3Select_i (i64, _r0)


#define d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                   \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                   \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
nextOp ();                                   \
}

#if d_m3HasFloat
d_m3Select_f (f32, _fp0, r, _r0)
d_m3Select_f (f32, _fp0, s, slot (i32))

d_m3Select_f (f64, _fp0, r, _r0)
d_m3Select_f (f64, _fp0, s, slot (i32))
#endif

d_m3Op  (Return){
    m3StackCheck();
    return m3Err_none;
}

DEBUG_TYPE WASM_DEBUG_BanchIf = WASM_DEBUG_ALL || (WASM_DEBUG && false);

d_m3Op  (BranchIf_r)
{
    i32 condition   = (i32) _r0;
    pc_t branch     = immediate (pc_t);

    if(WASM_DEBUG_BanchIf) ESP_LOGI("WASM3", "BranchIf_r: condition = %d, branch = %p", condition, branch);

    if (condition)
    {
        jumpOp (branch);
    }
    else nextOp ();
}


d_m3Op  (BranchIf_s)
{
    i32 condition   = slot (i32);
    pc_t branch     = immediate (pc_t);

    if(WASM_DEBUG_BanchIf) ESP_LOGI("WASM3", "BranchIf_s: condition = %d, branch = %p", condition, branch);

    if (condition)
    {
        jumpOp (branch);
    }
    else nextOp ();
}


d_m3Op  (BranchIfPrologue_r)
{
    i32 condition   = (i32) _r0;
    pc_t branch     = immediate (pc_t);

    if(WASM_DEBUG_BanchIf) ESP_LOGI("WASM3", "BranchIfPrologue_r: condition = %d, branch = %p", condition, branch);

    if (condition)
    {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        nextOp ();
    }
    else jumpOp (branch); // jump over the prologue
}


d_m3Op  (BranchIfPrologue_s)
{
    i32 condition   = slot (i32);
    pc_t branch     = immediate (pc_t);

    if(WASM_DEBUG_BanchIf) ESP_LOGI("WASM3", "BranchIfPrologue_s: condition = %d, branch = %p", condition, branch);

    if (condition)
    {
        nextOp ();
    }
    else jumpOp (branch);
}

DEBUG_TYPE WASM_DEBUG_ContinueLoop = WASM_DEBUG_ALL || (WASM_DEBUG && false);
d_m3Op  (ContinueLoop)
{
    if(WASM_DEBUG_ContinueLoop) ESP_LOGI("WASM3", "ContinueLoop called");

    m3StackCheck();

    // TODO: this is where execution can "escape" the M3 code and callback to the client / fiber switch
    // OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
    // has the potential to increase its native-stack usage. (don't forget ContinueLoopIf too.)

    void * loopId = immediate (void *);
    return loopId;
}


DEBUG_TYPE WASM_DEBUG_ContinueLoopIf = WASM_DEBUG_ALL || (WASM_DEBUG && false);
d_m3Op  (ContinueLoopIf)
{
    if(WASM_DEBUG_ContinueLoopIf) ESP_LOGI("WASM3", "ContinueLoopIf called");

    i32 condition = (i32) _r0;
    void * loopId = immediate (void *);

    if(WASM_DEBUG_ContinueLoopIf) ESP_LOGI("WASM3", "ContinueLoopIf: condition: %d, loopId: %p", condition, loopId);

    if (condition)
    {
        if(WASM_DEBUG_ContinueLoopIf) ESP_LOGI("WASM3", "ContinueLoopIf: return loopId: %p", loopId);
        return loopId;
    }
    else {
        if(WASM_DEBUG_ContinueLoopIf) ESP_LOGI("WASM3", "ContinueLoopIf: else nextOp()");
        nextOp ();
    }
}
 
////
////
////

//todo: deprecate it
#define MEMACCESS_SAFE(type, mem, offset) \
    ({ \
        void* ptr = m3SegmentedMemAccess(mem, offset, sizeof(type)); \
        if (!ptr) { \
            ESP_LOGE("WASM3", "Memory access failed at offset %u", (unsigned)offset); \
            return m3Err_pointerOverflow; \
        } \
        *(type*)ptr; \
    })

DEBUG_TYPE WASM_DEBUG_Const = WASM_DEBUG_ALL || (WASM_DEBUG && false); // Const32 and Const64
bool WASM_ConstUseComplexAssing = false;

d_m3Op (Const32) {
    /* Simply Also Known As
    u32 value = * (u32 *)_pc++;
    slot (u32) = value;
    nextOp ();
    */
    if(WASM_DEBUG_Const){ 
        ESP_LOGI("WASM3", "Const32 called");
        waitForIt();
    }

    u32 value = MEMACCESS(u32, _mem, _pc++);

    if(WASM_ConstUseComplexAssing){
        i32 imm = immediate(i32);    
        m3stack_t dest_offset = _sp + imm;
        if(WASM_DEBUG_Const) ESP_LOGW("WASM3", "Const32: _sp = %p, dest_offset = %p, imm = %d", _sp, dest_offset, imm);
        u32* dest = m3SegmentedMemAccess(_mem, CAST_PTR dest_offset, sizeof(u32));
        
        bool isErr = (dest == ERROR_POINTER);
        if (isErr != NULL) {
            ESP_LOGW("WASM3", "Destination memory failed at sp=%u, immediate=%d, dest=%p, _pc=%p", _sp, imm, dest, _pc);
            waitForIt();
            if(isErr) return m3Err_pointerOverflow;
        }
        
        if(WASM_DEBUG_Const){ 
            ESP_LOGI("WASM3", "Const32: assignment to %p of value %d", dest, value);
            waitForIt();
        }

        *dest = value;
        if(WASM_DEBUG_Const) ESP_LOGI("WASM3", "Const32: assignment done");
    }
    else {
        slot(u32) = value;
    }

    if(WASM_DEBUG_Const) ESP_LOGI("WASM3", "Const32: assigning value %d", value);

    nextOp();
}

DEBUG_TYPE WASM_Const64_CheckAlignment = false;
d_m3Op (Const64) {
    /* Simply Also Known As
    u64 value = * (u64 *)_pc;
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    slot (u64) = value;
    nextOp ();
    */
    if(WASM_DEBUG_Const) ESP_LOGI("WASM3", "Const64 called");

    // Prima verifica la validità dell'accesso alla memoria sorgente
    void* src_ptr = m3SegmentedMemAccess(_mem, _pc, sizeof(u64));

    // Leggi il valore usando memcpy per evitare problemi di allineamento
    u64 value = 0;

    bool isErr = (value == ERROR_POINTER);
    if (WASM_DEBUG_Const || isErr) {
        ESP_LOGI("WASM3", "Source memory access failed at pc=%u", (unsigned)_pc);
        if(isErr) return m3Err_mallocFailed;
    }
    
    m3_memcpy(_mem, &value, src_ptr, sizeof(u64));     

        // aka immediate(i32);
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;  // Su ESP32 sempre 2 perché M3_SIZEOF_PTR == 4

    if(WASM_ConstUseComplexAssing){
        // Calcola l'offset di destinazione
        m3slot_t imm = immediate(m3slot_t);
        m3stack_t dest_offset = _sp + imm;
        
        // Verifica l'accesso alla memoria di destinazione
        void* dest = m3SegmentedMemAccess(_mem, dest_offset, sizeof(u64));
        if (WASM_DEBUG_Const || dest == ERROR_POINTER) {
            ESP_LOGW("WASM3", "Destination memory access at sp=%u, immediate=%d, dest=%p", _sp, imm, dest);
            return m3Err_pointerOverflow;
        }

        // Verifica allineamento su ESP32
        if (WASM_Const64_CheckAlignment && (uintptr_t)dest & 7) {
            ESP_LOGW("WASM3", "Unaligned 64-bit access at offset %u", dest_offset);
        }

        // Copia il valore usando sempre memcpy per sicurezza
        m3_memcpy(_mem, dest, &value, sizeof(u64));
    }
    else {
        slot(u64) = value;
    }

    if(WASM_DEBUG_Const) ESP_LOGI("WASM3", "Const64: assigning value %d", value);

    nextOp();
}

d_m3Op  (Unsupported)
{
    newTrap ("unsupported instruction executed");
}

d_m3Op  (Unreachable)
{
    m3StackCheck();
    newTrap (m3Err_trapUnreachable);
}


d_m3Op  (End)
{
    m3StackCheck();
    return m3Err_none;
}


d_m3Op  (SetGlobal_s32)
{
    u32 * global = immediate (u32 *);
    * global = slot (u32);

    nextOp ();
}


d_m3Op  (SetGlobal_s64)
{
    u64 * global = immediate (u64 *);
    * global = slot (u64);

    nextOp ();
}

#if d_m3HasFloat
d_m3Op  (SetGlobal_f32)
{
    f32 * global = immediate (f32 *);
    * global = _fp0;

    nextOp ();
}


d_m3Op  (SetGlobal_f64)
{
    f64 * global = immediate (f64 *);
    * global = _fp0;

    nextOp ();
}
#endif


#define d_m3SkipMemoryBoundsCheck 1
#if d_m3SkipMemoryBoundsCheck
#  define m3MemCheck(x) true
#else
#  define m3MemCheck(x) M3_LIKELY(x)
#endif

#define d_m3Load(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = immediate (u32);                       \
    u64 operand = (u32) _r0;                            \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (SRC_TYPE) <= _mem->length     \
    )) {                                                \
        {                                               \
            u8* src8 = m3MemData(_mem) + operand;       \
            SRC_TYPE value;                             \
            m3_memcpy(_mem, &value, src8, sizeof(SRC_TYPE));\
            M3_BSWAP_##SRC_TYPE(value);                 \
            REG = (DEST_TYPE)value;                     \
            d_m3TraceLoad(DEST_TYPE, operand, REG);     \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (SRC_TYPE) <= _mem->length     \
    )) {                                                \
        {                                               \
            u8* src8 = m3MemData(_mem) + operand;       \
            SRC_TYPE value;                             \
            m3_memcpy(_mem, &value, src8, sizeof(SRC_TYPE));\
            M3_BSWAP_##SRC_TYPE(value);                 \
            REG = (DEST_TYPE)value;                     \
            d_m3TraceLoad(DEST_TYPE, operand, REG);     \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}

#define d_m3Load_i(DEST_TYPE, SRC_TYPE) d_m3Load(_r0, DEST_TYPE, SRC_TYPE)
#define d_m3Load_f(DEST_TYPE, SRC_TYPE) d_m3Load(_fp0, DEST_TYPE, SRC_TYPE)

#if d_m3HasFloat
d_m3Load_f (f32, f32);
d_m3Load_f (f64, f64);
#endif

d_m3Load_i (i32, i8);
d_m3Load_i (i32, u8);
d_m3Load_i (i32, i16);
d_m3Load_i (i32, u16);
d_m3Load_i (i32, i32);

d_m3Load_i (i64, i8);
d_m3Load_i (i64, u8);
d_m3Load_i (i64, i16);
d_m3Load_i (i64, u16);
d_m3Load_i (i64, i32);
d_m3Load_i (i64, u32);
d_m3Load_i (i64, i64);


///
/// Segmented memory store
///

#define d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, REG);     \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            m3_memcpy(_mem, mem8, &val, sizeof(val));   \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    const SRC_TYPE value = slot (SRC_TYPE);             \
    u64 operand = (u32) _r0;                            \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, value);   \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) value;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            m3_memcpy(_mem, mem8, &val, sizeof(val));   \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    const SRC_TYPE value = slot (SRC_TYPE);             \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, value);   \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) value;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            m3_memcpy(_mem, mem8, &val, sizeof(val));   \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}

// both operands can be in regs when storing a float
#define d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = (u32) _r0;                            \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (TYPE) <= _mem->length         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, operand, REG);         \
            u8* mem8 = m3MemData(_mem) + operand;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            m3_memcpy(_mem, mem8, &val, sizeof(val));   \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}

#define d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#define d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#if d_m3HasFloat
d_m3Store_f (f32, f32)
d_m3Store_f (f64, f64)
#endif

d_m3Store_i (i32, u8)
d_m3Store_i (i32, i16)
d_m3Store_i (i32, i32)

d_m3Store_i (i64, u8)
d_m3Store_i (i64, i16)
d_m3Store_i (i64, i32)
d_m3Store_i (i64, i64)

#undef m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#if d_m3EnableOpTracing
d_m3RetSig  debugOp  (d_m3OpSig, OP_TRACE_TYPE i_operationName)
{
    #if M3_FUNCTIONS_ENUM
    ESP_LOGI("WASM3", "Tracing op: %d %s", i_operationName, getOpName(i_operationName));
    #else 
    ESP_LOGI("WASM3", "Tracing op: %d %s", i_operationName, i_operationName);
    #endif

    nextOpDirect();
}
# endif

# if d_m3EnableOpProfiling
d_m3RetSig  profileOp  (d_m3OpSig, OP_TRACE_TYPE i_operationName)
{
    ProfileHit (i_operationName);

    nextOpDirect();
}
# endif

# if d_m3EnableOpTracing || d_m3EnableOpProfiling
d_m3RetSig traceOp(d_m3OpSig, OP_TRACE_TYPE opName){
    # if d_m3EnableOpTracing
        return debugOp(d_m3OpAllArgs, opName);
    # endif

    # if d_m3EnableOpProfiling
        return profileOp(d_m3OpAllArgs, opName);
    # endif
}
#endif

d_m3EndExternC

