//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#pragma once

#include "m3_core.h"
#include "m3_op_names_generated.h"

d_m3BeginExternC

//# define m3MemData(mem)                 (u8*)(((M3MemoryPoint*)(mem))->offset + 1) //todo: get memory at offset
//# define m3MemRuntime(mem)              (((M3Memory*)(mem))->runtime)
//# define m3MemInfo(mem)                 (&(((M3Memory*)(mem))->runtime->memory))

// Ottiene i dati del primo segmento (per compatibilità con codice legacy)
//#define m3MemData(mem)                 ((u8*)(((IM3Memory)(mem))->segments[0]->data))
#define m3MemData(mem)                 ((u8*)(0)) // just zero

// Ottiene il runtime associato alla memoria
#define m3MemRuntime(mem)              (((IM3Memory)(mem))->runtime)

// Ottiene un puntatore alla struttura di memoria stessa
#define m3MemInfo(mem)                 ((IM3Memory)(mem))

///
/// Segmented memory management
///

// Deprecated: direct memory access impossible with segmentation
//# define m3MemData(mem)                 m3SegmentedMemAccess((M3Memory*)(mem), 0, ((M3Memory*)(mem))->total_size)

// Helper macro per accesso sicuro a offset specifici
# define m3MemAccessAt(mem, off, sz)   m3SegmentedMemAccess((M3Memory*)(mem), (off), (sz))

#if TRACK_MEMACCESS && 0 // disabled
    #define STRINGIFY(x) #x

    #define MEMACCESS(type, mem, pc) \
        (ESP_LOGI("WASM3", "MEM ACCESS type: %s\n", STRINGIFY(type)), \
        *((type*)(m3SegmentedMemAccess(mem, pc, sizeof(type)))))

    #define MEMPOINT(type, mem, pc) \
        (ESP_LOGI("WASM3", "MEM POINT type: %s\n", STRINGIFY(type))), \
        (type*)m3SegmentedMemAccess(mem, pc, sizeof(type))

#else
#define MEMACCESS(type, mem, pc) \
    *(type*)m3SegmentedMemAccess(mem, pc, sizeof(type))

#define MEMPOINT(type, mem, pc) \
    (type)m3SegmentedMemAccess(mem, pc, sizeof(type))
#endif

///
///
///

# define d_m3BaseOpSig                   pc_t _pc, m3stack_t _sp, M3Memory * _mem, m3reg_t _r0

# define d_m3BaseOpArgs                 _sp, _mem, _r0
# define d_m3BaseOpAllArgs              _pc, _sp, _mem, _r0
# define d_m3BaseOpDefaultArgs          0
# define d_m3BaseClearRegisters         _r0 = 0;

#if M3_FUNCTIONS_ENUM
# define d_m3BaseCstr                   -3
#else
# define d_m3BaseCstr                   ""
#endif

# define d_m3ExpOpSig(...)              d_m3BaseOpSig, __VA_ARGS__
# define d_m3ExpOpArgs(...)             d_m3BaseOpArgs, __VA_ARGS__
# define d_m3ExpOpAllArgs(...)          d_m3BaseOpAllArgs, __VA_ARGS__
# define d_m3ExpOpDefaultArgs(...)      d_m3BaseOpDefaultArgs, __VA_ARGS__
# define d_m3ExpClearRegisters(...)     d_m3BaseClearRegisters; __VA_ARGS__

# if d_m3HasFloat
#   define d_m3OpSig                d_m3ExpOpSig            (f64 _fp0)
#   define d_m3OpArgs               d_m3ExpOpArgs           (_fp0)
#   define d_m3OpAllArgs            d_m3ExpOpAllArgs        (_fp0)
#   define d_m3OpDefaultArgs        d_m3ExpOpDefaultArgs    (0.)
#   define d_m3ClearRegisters       d_m3ExpClearRegisters   (_fp0 = 0.;)
# else
#   define d_m3OpSig                d_m3BaseOpSig
#   define d_m3OpArgs               d_m3BaseOpArgs
#   define d_m3OpAllArgs            d_m3BaseOpAllArgs
#   define d_m3OpDefaultArgs        d_m3BaseOpDefaultArgs
#   define d_m3ClearRegisters       d_m3BaseClearRegisters
# endif

// Original
//  typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig);
//  #define d_m3RetSig                  static inline m3ret_t vectorcall
//  #define d_m3Op(NAME)                M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#define d_m3RetSig_NOINLINE 0
#if d_m3RetSig_NOINLINE
#define d_m3RetSig_ATTR static NOINLINE_ATTR
#else 
#define d_m3RetSig_ATTR static inline
#endif

#if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig, OP_TRACE_TYPE i_operationName);
    #define d_m3RetSig      d_m3RetSig_ATTR m3ret_t vectorcall
    #define d_m3Op(NAME) M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, OP_TRACE_TYPE i_operationName)    
    

    #define TRACE_FUNC_NAME , i_operationName

    #if M3_FUNCTIONS_ENUM
        #define TRACE_NAME getOpName(i_operationName)
    #else
        #define TRACE_NAME i_operationName
    #endif
    
#else
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig);
    #define d_m3RetSig      d_m3RetSig_ATTR m3ret_t vectorcall
    #define d_m3Op(NAME) M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

    #define TRACE_FUNC_NAME
    #define TRACE_NAME __FUNCTION__
#endif

#if M3Runtime_Stack_Segmented
    #if WASM_ENABLE_OP_TRACE
        #define nextOpImpl() ({ \
            M3Result result; \
            if (trace_context.current_stack_depth >= TRACE_STACK_DEPTH_MAX) { \
                result = m3Err_trapStackOverflow; \
            } else { \
                IM3Operation op = (MEMACCESS(IM3Operation, _mem, _pc)); \
                trace_enter(op, trace_context.current_stack_depth, TRACE_NAME); \
                trace_context.current_stack_depth++; \
                result = op(_pc + 1, d_m3OpArgs TRACE_FUNC_NAME); \
                trace_context.current_stack_depth--; \
                trace_exit(op, trace_context.current_stack_depth, TRACE_NAME); \
            } \
            result; \
        })

        #define jumpOpImpl(PC) ({ \
            M3Result result; \
            if (trace_context.current_stack_depth >= TRACE_STACK_DEPTH_MAX) { \
                result = m3Err_trapStackOverflow; \
            } else { \
                IM3Operation op = (MEMACCESS(IM3Operation, _mem, PC)); \
                trace_enter(op, trace_context.current_stack_depth, TRACE_NAME); \
                trace_context.current_stack_depth++; \
                result = op(PC + 1, d_m3OpArgs TRACE_FUNC_NAME); \
                trace_context.current_stack_depth--; \
                trace_exit(op, trace_context.current_stack_depth, TRACE_NAME); \
            } \
            result; \
        })
    #else
        #define nextOpImpl() ((MEMACCESS(IM3Operation, _mem, _pc))(_pc + 1, d_m3OpArgs TRACE_FUNC_NAME))
        #define jumpOpImpl(PC) ((MEMACCESS(IM3Operation, _mem, PC))(PC + 1, d_m3OpArgs TRACE_FUNC_NAME))
    #endif
#else
    #define nextOpImpl() ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs TRACE_FUNC_NAME)
    #define jumpOpImpl(PC) ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs TRACE_FUNC_NAME)
#endif

#define AVOID_M3_MUSTTAIL 0
#if AVOID_M3_MUSTTAIL
    #undef M3_MUSTTAIL
    #define M3_MUSTTAIL 
#endif 

// Original
#define nextOpDirect()              M3_MUSTTAIL return nextOpImpl()
#define jumpOpDirect(PC)            M3_MUSTTAIL return jumpOpImpl((pc_t)(PC))

#if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    #if M3_FUNCTIONS_ENUM
        d_m3RetSig RunCode (d_m3OpSig, OP_TRACE_TYPE i_operationName)
    #else
        d_m3RetSig RunCode (d_m3OpSig, cstr_t i_operationName)
    #endif
    {
        nextOpDirect();
    }
#else
    d_m3RetSig RunCode (d_m3OpSig)
    {
        nextOpDirect();
    }
#endif