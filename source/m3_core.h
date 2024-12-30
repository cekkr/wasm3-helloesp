//
//  m3_core.h
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "wasm3.h"
#include "m3_exception.h"
#include "m3_segmented_memory.h"


typedef struct M3MemoryHeader
{
    IM3Runtime      runtime;
    void *          maxStack;
    size_t          length;
}
M3MemoryHeader;

typedef void *                              code_t; // was const void *
typedef code_t const *                      pc_t; // was const * /*__restrict__*/

#define d_m3CodePageFreeLinesThreshold      4+2       // max is: select _sss & CallIndirect + 2 for bridge

#define d_m3DefaultMemPageSize              65536

#define d_m3Reg0SlotAlias                   60000
#define d_m3Fp0SlotAlias                    (d_m3Reg0SlotAlias + 2)

#define d_m3MaxSaneTypesCount               1000000
#define d_m3MaxSaneFunctionsCount           1000000
#define d_m3MaxSaneImportsCount             100000
#define d_m3MaxSaneExportsCount             100000
#define d_m3MaxSaneGlobalsCount             1000000
#define d_m3MaxSaneElementSegments          10000000
#define d_m3MaxSaneDataSegments             100000
#define d_m3MaxSaneTableSize                10000000
#define d_m3MaxSaneUtf8Length               10000
#define d_m3MaxSaneFunctionArgRetCount      1000    // still insane, but whatever

#define d_externalKind_function             0
#define d_externalKind_table                1
#define d_externalKind_memory               2
#define d_externalKind_global               3

static const char * const c_waTypes []          = { "nil", "i32", "i64", "f32", "f64", "unknown" };
static const char * const c_waCompactTypes []   = { "_", "i", "I", "f", "F", "?" };


# if d_m3VerboseErrorMessages

M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  const char * const i_file, u32 i_lineNum, const char * const i_errorMessage, ...);

#  define _m3Error(RESULT, RT, MOD, FUN, FILE, LINE, FORMAT, ...) \
            m3Error (RESULT, RT, MOD, FUN, FILE, LINE, FORMAT, ##__VA_ARGS__)

# else
#  define _m3Error(RESULT, RT, MOD, FUN, FILE, LINE, FORMAT, ...) (RESULT)
# endif

#define ErrorRuntime(RESULT, RUNTIME, FORMAT, ...)      _m3Error (RESULT, RUNTIME, NULL, NULL,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#define ErrorModule(RESULT, MOD, FORMAT, ...)           _m3Error (RESULT, MOD->runtime, MOD, NULL,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#define ErrorCompile(RESULT, COMP, FORMAT, ...)         _m3Error (RESULT, COMP->runtime, COMP->module, NULL, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)

#if d_m3LogNativeStack
void        m3StackCheckInit        ();
void        m3StackCheck            ();
int         m3StackGetMax           ();
#else
#define     m3StackCheckInit()
#define     m3StackCheck()
#define     m3StackGetMax()         0
#endif

#if d_m3LogTimestamps
#define     PRIts                   "%llu"
uint64_t    m3_GetTimestamp         ();
#else
#define     PRIts                   "%s"
#define     m3_GetTimestamp()       ""
#endif

//todo: reimplement m3_Malloc_Impl, m3_Realloc_Impl, m3_Free_Impl, m3_CopyMem
void        m3_Abort                (const char* message);
void *      m3_Malloc_Impl          (size_t i_size);
void *      m3_Realloc_Impl         (void * i_ptr, size_t i_newSize, size_t i_oldSize);
void        m3_Free_Impl            (void * i_ptr, bool isMemory);
void *      m3_CopyMem              (const void * i_from, size_t i_size);

#if d_m3LogHeapOps

// Tracing format: timestamp;heap:OpCode;name;size(bytes);new items;new ptr;old items;old ptr

static inline void * m3_AllocStruct_Impl(ccstr_t name, size_t i_size) {
    void * result = m3_Malloc_Impl(i_size);
    fprintf(stderr, PRIts ";heap:AllocStruct;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_size, result);
    return result;
}

static inline void * m3_AllocArray_Impl(ccstr_t name, size_t i_num, size_t i_size) {
    void * result = m3_Malloc_Impl(i_size * i_num);
    fprintf(stderr, PRIts ";heap:AllocArr;%s;%zu;%zu;%p;;\n", m3_GetTimestamp(), name, i_size, i_num, result);
    return result;
}

static inline void * m3_ReallocArray_Impl(ccstr_t name, void * i_ptr_old, size_t i_num_new, size_t i_num_old, size_t i_size) {
    void * result = m3_Realloc_Impl (i_ptr_old, i_size * i_num_new, i_size * i_num_old);
    fprintf(stderr, PRIts ";heap:ReallocArr;%s;%zu;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_size, i_num_new, result, i_num_old, i_ptr_old);
    return result;
}

static inline void * m3_Malloc (ccstr_t name, size_t i_size) {
    void * result = m3_Malloc_Impl (i_size);
    fprintf(stderr, PRIts ";heap:AllocMem;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_size, result);
    return result;
}
static inline void * m3_Realloc (ccstr_t name, void * i_ptr, size_t i_newSize, size_t i_oldSize) {
    void * result = m3_Realloc_Impl (i_ptr, i_newSize, i_oldSize);
    fprintf(stderr, PRIts ";heap:ReallocMem;%s;;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_newSize, result, i_oldSize, i_ptr);
    return result;
}

#endif
// there was an #else statement


M3Result    NormalizeType           (u8 * o_type, i8 i_convolutedWasmType);

bool        IsIntType               (u8 i_wasmType);
bool        IsFpType                (u8 i_wasmType);
bool        Is64BitType             (u8 i_m3Type);
u32         SizeOfType              (u8 i_m3Type);

M3Result    Read_u64                (IM3Memory memory, u64 * o_value, bytes_t * io_bytes, cbytes_t i_end);
M3Result    Read_u32                (IM3Memory memory, u32 * o_value, bytes_t * io_bytes, cbytes_t i_end);
#if d_m3ImplementFloat
M3Result    Read_f64                (IM3Memory memory, f64 * o_value, bytes_t * io_bytes, cbytes_t i_end);
M3Result    Read_f32                (IM3Memory memory, f32 * o_value, bytes_t * io_bytes, cbytes_t i_end);
#endif
M3Result    Read_u8                 (IM3Memory memory, u8  * o_value, bytes_t * io_bytes, cbytes_t i_end);
M3Result    Read_opcode             (IM3Memory memory, m3opcode_t * o_value, bytes_t  * io_bytes, cbytes_t i_end);

M3Result    ReadLebUnsigned         (IM3Memory memory, u64 * o_value, u32 i_maxNumBits, bytes_t * io_bytes, cbytes_t i_end);
M3Result    ReadLebSigned           (IM3Memory memory, i64 * o_value, u32 i_maxNumBits, bytes_t * io_bytes, cbytes_t i_end);
M3Result    ReadLEB_u32             (IM3Memory memory, u32 * o_value, bytes_t * io_bytes, cbytes_t i_end);
M3Result ReadLEB_ptr(IM3Memory memory, m3stack_t o_value, bytes_t* io_bytes, cbytes_t i_end) ;
M3Result    ReadLEB_u7              (IM3Memory memory, u8  * o_value, bytes_t * io_bytes, cbytes_t i_end);
M3Result    ReadLEB_i7              (IM3Memory memory, i8  * o_value, bytes_t * io_bytes, cbytes_t i_end);
M3Result    ReadLEB_i32             (IM3Memory memory, i32 * o_value, bytes_t * io_bytes, cbytes_t i_end);
M3Result    ReadLEB_i64             (IM3Memory memory, i64 * o_value, bytes_t * io_bytes, cbytes_t i_end);
M3Result    Read_utf8               (IM3Memory memory, cstr_t * o_utf8, bytes_t * io_bytes, cbytes_t i_end);

cstr_t      SPrintValue             (void * i_value, u8 i_type);
size_t      SPrintArg               (char * o_string, size_t i_stringBufferSize, voidptr_t i_sp, u8 i_type);

void        ReportError             (IM3Runtime io_runtime, IM3Module i_module, IM3Function i_function, ccstr_t i_errorMessage, ccstr_t i_file, u32 i_lineNum);

# if d_m3RecordBacktraces
void        PushBacktraceFrame         (IM3Runtime io_runtime, pc_t i_pc);
void        FillBacktraceFunctionInfo  (IM3Runtime io_runtime, IM3Function i_function);
void        ClearBacktrace             (IM3Runtime io_runtime);
# endif

void *  m3_Int_CopyMem  (const void * i_from, size_t i_size);

void* default_malloc(size_t size);
void default_free(void* ptr);
void* default_realloc(void* ptr, size_t new_size);
//bool allocate_segment(M3Memory* memory, size_t segment_index);

typedef struct {
    void* (*malloc)(size_t size);
    void  (*free)(void* ptr);
    void* (*realloc)(void* ptr, size_t new_size);
} MemoryAllocator;

static MemoryAllocator default_allocator = {
    .malloc = default_malloc,
    .free = default_free,
    .realloc = default_realloc
};

static MemoryAllocator* current_allocator = &default_allocator;

d_m3EndExternC
