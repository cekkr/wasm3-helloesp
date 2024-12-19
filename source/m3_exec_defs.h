//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_exec_defs_h
#define m3_exec_defs_h

#include "m3_core.h"

d_m3BeginExternC


//# define m3MemData(mem)                 (u8*)(((M3MemoryPoint*)(mem))->offset) //todo: get memory at offset
# define m3MemRuntime(mem)              (((M3Memory*)(mem))->runtime)
# define m3MemInfo(mem)                 (&(((M3Memory*)(mem))->runtime->memory))

///
/// Segmented memory management
///

// Deprecated: direct memory access impossible with segmentation
//# define m3MemData(mem)                 m3SegmentedMemAccess((M3Memory*)(mem), 0, ((M3Memory*)(mem))->total_size)

// Helper macro per accesso sicuro a offset specifici
# define m3MemAccessAt(mem, off, sz)   m3SegmentedMemAccess((M3Memory*)(mem), (off), (sz))

//#define TRACK_MEMACCESS
#ifdef TRACK_MEMACCESS
#define STRINGIFY(x) #x
#define MEMACCESS(type, mem, pc) \    
    (printf("MEM ACCESS type: %s\n", STRINGIFY(type)), \
    *((type*)(m3SegmentedMemAccess(mem, pc, sizeof(type)))))
#else
#define MEMACCESS(type, mem, pc) \
    *(type*)m3SegmentedMemAccess(mem, pc, sizeof(type))

#define MEMPOINT(type, mem, pc) \
    (pc_t)m3SegmentedMemAccess(mem, pc, sizeof(type))
#endif
 

///
///
///

//#define OPERTATIONS_ON_SEGMENTED_MEM

#ifdef OPERTATIONS_ON_SEGMENTED_MEM
// iptr deprecated
# define d_m3BaseOpSig                  iptr _pc, m3stack_t _sp, M3Memory * _mem, m3reg_t _r0
#else
# define d_m3BaseOpSig                  pc_t _pc, m3stack_t _sp, M3Memory * _mem, m3reg_t _r0
#endif


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


#define d_m3RetSig                  static inline m3ret_t vectorcall

#define OPERTATIONS_ON_SEGMENTED_MEM 1
#if OPERTATIONS_ON_SEGMENTED_MEM

#if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig, cstr_t i_operationName);
#    define d_m3Op(NAME)           M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#    define nextOpImpl()           ({ \
                                      IM3Operation op = (IM3Operation)m3SegmentedMemAccess(_mem, _pc, sizeof(IM3Operation)); \
                                      op(_pc + 1, d_m3OpArgs, __FUNCTION__); \
                                   })

#    define jumpOpImpl(PC)         ({ \
                                      IM3Operation op = (IM3Operation)m3SegmentedMemAccess(_mem, (PC), sizeof(IM3Operation)); \
                                      op((PC) + 1, d_m3OpArgs, __FUNCTION__); \
                                   })
#else
typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig);

#    define d_m3Op(NAME)           M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#    define nextOpImpl()           ({ \
                                      IM3Operation op = (IM3Operation)m3SegmentedMemAccess(_mem, _pc, sizeof(IM3Operation)); \
                                      op(_pc + 1, d_m3OpArgs); \
                                   })

#    define jumpOpImpl(PC)         ({ \
                                      IM3Operation op = (IM3Operation)m3SegmentedMemAccess(_mem, (PC), sizeof(IM3Operation)); \
                                      op((PC) + 1, d_m3OpArgs); \
                                   })
#endif

#define nextOpDirect()              return nextOpImpl()
#define jumpOpDirect(PC)            return jumpOpImpl((PC))

#else 

#if M3Runtime_Stack_Segmented

// Tracing defintion missing
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig);
#    define d_m3Op(NAME)                M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#    define nextOpImpl()            ((IM3Operation)(* MEMPOINT(IM3Operation, _mem, _pc)))(_pc + 1, d_m3OpArgs)
#    define jumpOpImpl(PC)          ((IM3Operation)(* MEMPOINT(IM3Operation, _mem,  PC)))( PC + 1, d_m3OpArgs)

#else

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig, cstr_t i_operationName);
#    define d_m3Op(NAME)                M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#    define nextOpImpl()            ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs, __FUNCTION__)
#    define jumpOpImpl(PC)          ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs, __FUNCTION__)
# else
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig);
#    define d_m3Op(NAME)                M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#    define nextOpImpl()            ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs)
#    define jumpOpImpl(PC)          ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs)
# endif
#endif

#endif

#define nextOpDirect()              M3_MUSTTAIL return nextOpImpl()
#define jumpOpDirect(PC)            M3_MUSTTAIL return jumpOpImpl((pc_t)(PC))

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig  RunCode  (d_m3OpSig, cstr_t i_operationName)
# else
d_m3RetSig  RunCode  (d_m3OpSig)
# endif
{
    nextOpDirect();
}

d_m3EndExternC

#endif // m3_exec_defs_h
