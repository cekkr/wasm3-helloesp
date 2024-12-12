//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#pragma once

#include "m3_segmented_memory.h"
#include "m3_core.h"   

d_m3BeginExternC

//typedef double f64;

/*
# define m3MemData(mem)                 (u8*)(((M3MemoryHeader*)(mem))+1)
# define m3MemData(mem)                 (u8*)(((M3MemoryHeader*)(mem))) // useless subtitutive macro
# define m3MemRuntime(mem)              (((M3MemoryHeader*)(mem))->runtime)
# define m3MemInfo(mem)                 (&(((M3MemoryHeader*)(mem))->runtime->memory))
*/

// M3MemoryHeader ignored for M3Memory
# define m3MemData(mem)                 (u8*)(((M3Memory*)(mem))+1)
# define m3MemData(mem)                 (u8*)(((M3Memory*)(mem))) // useless subtitutive macro
# define m3MemRuntime(mem)              (((M3Memory*)(mem))->runtime)
# define m3MemInfo(mem)                 (&(((M3Memory*)(mem))->runtime->memory))

typedef void* M3MemoryPoint_ptr; // it means M3MemoryPoint
typedef void* M3Memory_ptr; // it means M3Memory

# define d_m3BaseOpSig                  pc_t _pc, M3MemoryPoint_ptr _sp, M3Memory_ptr _mem, m3reg_t _r0
# define d_m3BaseOpArgs                 _sp, _mem, _r0
# define d_m3BaseOpAllArgs              _pc, _sp, _mem, _r0
# define d_m3BaseOpDefaultArgs          0
# define d_m3BaseClearRegisters         _r0 = 0;
# define d_m3BaseCstr                   ""

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

//#include "m3_env.h"

#define d_m3RetSig                  static inline m3ret_t vectorcall

/* Converted in static inline functions
# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig, cstr_t i_operationName);
#    define d_m3Op(NAME)                M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#    define nextOpImpl()            ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs, __FUNCTION__)
#    define jumpOpImpl(PC)          ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs, __FUNCTION__)
# else
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig); // was vectorcall * IM3Operation
#    define d_m3Op(NAME)                M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#   define nextOpImpl()             ((IM3Operation)(*_pc))(*_pc + 1, d_m3OpArgs)
#   define jumpOpImpl(PC)           ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs)
# endif
*/

typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig);

static inline m3ret_t nextOpImpl(d_m3OpSig) {
    return ((IM3Operation)(*_pc))(_pc + 1, _sp, _mem, _r0
    #if d_m3HasFloat
        , _fp0
    #endif
    );
}

static inline m3ret_t jumpOpImpl(pc_t PC, d_m3OpSig) {
    return ((IM3Operation)(*PC))(PC + 1, _sp, _mem, _r0
    #if d_m3HasFloat
        , _fp0
    #endif
    );
}

// Queste funzioni devono avere gli stessi argomenti delle originali
static inline m3ret_t nextOpDirect(d_m3OpSig) {
    return nextOpImpl(_pc, _sp, _mem, _r0
    #if d_m3HasFloat
        , _fp0
    #endif
    );
}

static inline m3ret_t jumpOpDirect(pc_t PC, d_m3OpSig) {
    return jumpOpImpl(PC, _pc, _sp, _mem, _r0
    #if d_m3HasFloat
        , _fp0
    #endif
    );
}

// Converted in static inline functions
//#define nextOpDirect()              M3_MUSTTAIL return nextOpImpl()
//#define jumpOpDirect(PC)            M3_MUSTTAIL return jumpOpImpl((pc_t)(PC))


#if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig RunCode(d_m3OpSig, cstr_t i_operationName)
#else
d_m3RetSig RunCode(d_m3OpSig)
#endif
{
    return nextOpImpl(_pc, _sp, _mem, _r0
    #if d_m3HasFloat
        , _fp0
    #endif
    );
}

d_m3EndExternC

