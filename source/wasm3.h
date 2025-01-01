//
//  Wasm3, high performance WebAssembly interpreter
//
//  Copyright © 2019 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#pragma once

#define M3_VERSION_MAJOR 0
#define M3_VERSION_MINOR 5
#define M3_VERSION_REV   1
#define M3_VERSION       "0.5.1"

#define DEBUG_TYPE static const bool

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PASSTHROUGH_HELLOESP 1
#if PASSTHROUGH_HELLOESP
#include "he_defines.h"

#include "esp_debug_helpers.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_cache.h"
#include "esp_task_wdt.h" // for watchdog reset
#include "esp_attr.h"

#endif

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#define M3Runtime_Stack_Segmented 1

#define M3CodePage_RemoveCodePageOfCapacity_FreePage 0

#define WASM_DEBUG 0
#define WASM_DEBUG_ALL 0

#if HELLOESP_WASM_DEBUG
#define WASM_DEBUG 1
#endif

#define DEBUG_MEMORY 0

#define WASM_ENABLE_OP_TRACE 0
#define WASM_TRACE_LOADSTORE 0
#define M3_FUNCTIONS_ENUM 1
#define TRACK_MEMACCESS 0

#define WASM_ENABLE_CHECK_MEMORY_PTR 0

#ifndef WASM_PTRS_64BITS
#define WASM_PTRS_64BITS 0
#endif

#if WASM_ENABLE_OP_TRACE
    //#define d_m3EnableOpTracing 1
    //#define d_m3EnableStrace 3 // enable with d_m3EnableOpTracing

    #define d_m3EnableOpProfiling 1 // problems with op_DumpStack reference (anyway, the same problem occurs without it)    
#endif

//#define d_m3EnableStrace 1

///
/// Architecture
///

# if WASM_PTRS_64BITS
#   define d_m3Use32BitSlots                    0
# else
#   define d_m3Use32BitSlots                    1
# endif

///
/// Memory allocation
///

// Probably useful when you activeate WASM_ENABLE_OP_TRACE
#define WASM_OPS_DISABLE_IRAM 1

// it's about m3_compile
#define DISABLE_WASM3_INLINE 0

#define ESP_OPS_ATTR DRAM_ATTR // Move ops from IRAM to DRAM

///
/// Debug
///
#if M3_FUNCTIONS_ENUM
#define OP_TRACE_TYPE int
#else
#define OP_TRACE_TYPE cstr_t
#endif

#define WASM_DEBUG_DumpStack_InOps 0

///
/// Operations index
///
#include "wasm3_defs.h"
#include "m3_config.h"
#include "m3_op_names_generated.h"

#include "m3_debug.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

// Constants
#define M3_BACKTRACE_TRUNCATED      (IM3BacktraceFrame)(SIZE_MAX)

#if defined(__cplusplus)
extern "C" {
#endif

typedef const char *    M3Result;

struct M3Environment;   typedef struct M3Environment *  IM3Environment;
struct M3Runtime;       typedef struct M3Runtime *      IM3Runtime;
struct M3Module;        typedef struct M3Module *       IM3Module;
struct M3Function;      typedef struct M3Function *     IM3Function;
struct M3Global;        typedef struct M3Global *       IM3Global;

typedef struct M3ErrorInfo
{
    M3Result        result;

    IM3Runtime      runtime;
    IM3Module       module;
    IM3Function     function;

    const char *    file;
    uint32_t        line;

    const char *    message;
} M3ErrorInfo;

typedef struct M3BacktraceFrame
{
    uint32_t                     moduleOffset;
    IM3Function                  function;

    struct M3BacktraceFrame *    next;
}
M3BacktraceFrame, * IM3BacktraceFrame;

typedef struct M3BacktraceInfo
{
    IM3BacktraceFrame      frames;
    IM3BacktraceFrame      lastFrame;    // can be M3_BACKTRACE_TRUNCATED
}
M3BacktraceInfo, * IM3BacktraceInfo;


typedef enum M3ValueType
{
    c_m3Type_none   = 0,
    c_m3Type_i32    = 1,
    c_m3Type_i64    = 2,
    c_m3Type_f32    = 3,
    c_m3Type_f64    = 4,

    c_m3Type_unknown
} M3ValueType;

typedef struct M3TaggedValue
{
    M3ValueType type;
    union M3ValueUnion
    {
        uint32_t    i32;
        uint64_t    i64;
        float       f32;
        double      f64;
    } value;
}
M3TaggedValue, * IM3TaggedValue;

typedef struct M3ImportInfo
{
    const char *    moduleUtf8;
    const char *    fieldUtf8;
}
M3ImportInfo, * IM3ImportInfo;


typedef struct M3ImportContext
{
    void *          userdata;
    IM3Function     function;
}
M3ImportContext, * IM3ImportContext;

// -------------------------------------------------------------------------------------------------------------------------------
//  error codes
// -------------------------------------------------------------------------------------------------------------------------------

# if defined(M3_IMPLEMENT_ERROR_STRINGS)
#   if defined(__cplusplus)
#     define d_m3ErrorConst(LABEL, STRING)      extern const M3Result m3Err_##LABEL = { STRING };
#   else
#     define d_m3ErrorConst(LABEL, STRING)      const M3Result m3Err_##LABEL = { STRING };
#   endif
# else
#   define d_m3ErrorConst(LABEL, STRING)        static const M3Result m3Err_##LABEL;
# endif

// -------------------------------------------------------------------------------------------------------------------------------

d_m3ErrorConst  (none,                          NULL)

// general errors
d_m3ErrorConst  (mallocFailed,                  "memory allocation failed")

// parse errors
d_m3ErrorConst  (incompatibleWasmVersion,       "incompatible Wasm binary version")
d_m3ErrorConst  (wasmMalformed,                 "malformed Wasm binary")
d_m3ErrorConst  (malformedUtf8,             "malformed UTF-8")
d_m3ErrorConst  (misorderedWasmSection,         "out of order Wasm section")
d_m3ErrorConst  (wasmUnderrun,                  "underrun while parsing Wasm binary")
d_m3ErrorConst  (wasmOverrun,                   "overrun while parsing Wasm binary")
d_m3ErrorConst  (wasmMissingInitExpr,           "missing init_expr in Wasm binary")
d_m3ErrorConst  (lebOverflow,                   "LEB encoded value overflow")
d_m3ErrorConst  (missingUTF8,                   "invalid length UTF-8 string")
d_m3ErrorConst  (wasmSectionUnderrun,           "section underrun while parsing Wasm binary")
d_m3ErrorConst  (wasmSectionOverrun,            "section overrun while parsing Wasm binary")
d_m3ErrorConst  (invalidTypeId,                 "unknown value_type")
d_m3ErrorConst  (tooManyMemorySections,         "only one memory per module is supported")
d_m3ErrorConst  (tooManyArgsRets,               "too many arguments or return values")

// link errors
d_m3ErrorConst  (moduleNotLinked,               "attempting to use module that is not loaded")
d_m3ErrorConst  (moduleAlreadyLinked,           "attempting to bind module to multiple runtimes")
d_m3ErrorConst  (functionLookupFailed,          "function lookup failed")
d_m3ErrorConst  (functionImportMissing,         "missing imported function")

d_m3ErrorConst  (malformedFunctionSignature,    "malformed function signature")

// compilation errors
d_m3ErrorConst  (noCompiler,                    "no compiler found for opcode")
d_m3ErrorConst  (unknownOpcode,                 "unknown opcode")
d_m3ErrorConst  (restrictedOpcode,              "restricted opcode")
d_m3ErrorConst  (functionStackOverflow,         "compiling function overran its stack height limit")
d_m3ErrorConst  (functionStackUnderrun,         "compiling function underran the stack")
d_m3ErrorConst  (mallocFailedCodePage,          "memory allocation failed when acquiring a new M3 code page")
d_m3ErrorConst  (memoryLimit,                   "memory limit exceeded")
d_m3ErrorConst  (nullMemory,                    "null memory")
d_m3ErrorConst  (runtimeMemoryNotInit,          "runtime's memory is not initialized")
d_m3ErrorConst  (settingImmutableGlobal,        "attempting to set an immutable global")
d_m3ErrorConst  (typeMismatch,                  "incorrect type on stack")
d_m3ErrorConst  (typeCountMismatch,             "incorrect value count on stack")

// runtime errors
d_m3ErrorConst  (missingCompiledCode,           "function is missing compiled m3 code")
d_m3ErrorConst  (wasmMemoryOverflow,            "runtime ran out of memory")
d_m3ErrorConst  (pointerOverflow,               "pointer ran out of memory")
d_m3ErrorConst  (globalMemoryNotAllocated,      "global memory is missing from a module")
d_m3ErrorConst  (globaIndexOutOfBounds,         "global index is too large")
d_m3ErrorConst  (argumentCountMismatch,         "argument count mismatch")
d_m3ErrorConst  (argumentTypeMismatch,          "argument type mismatch")
d_m3ErrorConst  (globalLookupFailed,            "global lookup failed")
d_m3ErrorConst  (globalTypeMismatch,            "global type mismatch")
d_m3ErrorConst  (globalNotMutable,              "global is not mutable")
d_m3ErrorConst  (nullRuntime,                   "runtime is null")
d_m3ErrorConst  (nullSegmentData,               "unable to allocate segment data")
d_m3ErrorConst  (nullPointer,                   "null pointer")
d_m3ErrorConst  (malformedData,                  "malformed data")

// traps
d_m3ErrorConst  (trapOutOfBoundsMemoryAccess,   "[trap] out of bounds memory access")
d_m3ErrorConst  (trapDivisionByZero,            "[trap] integer divide by zero")
d_m3ErrorConst  (trapIntegerOverflow,           "[trap] integer overflow")
d_m3ErrorConst  (trapIntegerConversion,         "[trap] invalid conversion to integer")
d_m3ErrorConst  (trapIndirectCallTypeMismatch,  "[trap] indirect call type mismatch")
d_m3ErrorConst  (trapTableIndexOutOfRange,      "[trap] undefined element")
d_m3ErrorConst  (trapTableElementIsNull,        "[trap] null table element")
d_m3ErrorConst  (trapExit,                      "[trap] program called exit")
d_m3ErrorConst  (trapAbort,                     "[trap] program called abort")
d_m3ErrorConst  (trapUnreachable,               "[trap] unreachable executed")
d_m3ErrorConst  (trapStackOverflow,             "[trap] stack overflow")


//-------------------------------------------------------------------------------------------------------------------------------
//  configuration, can be found in m3_config.h, m3_config_platforms.h, m3_core.h)
//-------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------------------
//  global environment than can host multiple runtimes
//-------------------------------------------------------------------------------------------------------------------------------
    IM3Environment      m3_NewEnvironment           (void);

    void                m3_FreeEnvironment          (IM3Environment i_environment);

    typedef M3Result (* M3SectionHandler) (IM3Module i_module, const char* name, const uint8_t * start, const uint8_t * end);

    void                m3_SetCustomSectionHandler  (IM3Environment i_environment,    M3SectionHandler i_handler);


//-------------------------------------------------------------------------------------------------------------------------------
//  execution context
//-------------------------------------------------------------------------------------------------------------------------------

    IM3Runtime          m3_NewRuntime               (IM3Environment         io_environment,
                                                     uint32_t               i_stackSizeInBytes,
                                                     void *                 i_userdata);

    void                m3_FreeRuntime              (IM3Runtime             i_runtime);

    // Wasm currently only supports one memory region. i_memoryIndex should be zero.
    uint8_t *           m3_GetMemory                (IM3Runtime             i_runtime,
                                                     uint32_t *             o_memorySizeInBytes,
                                                     uint32_t               i_memoryIndex);

    // This is used internally by Raw Function helpers
    uint32_t            m3_GetMemorySize            (IM3Runtime             i_runtime);

    void *              m3_GetUserData              (IM3Runtime             i_runtime);


//-------------------------------------------------------------------------------------------------------------------------------
//  modules
//-------------------------------------------------------------------------------------------------------------------------------

    // i_wasmBytes data must be persistent during the lifetime of the module
    /*M3Result            m3_ParseModule              (IM3Environment         i_environment,
                                                     IM3Module *            o_module,
                                                     const uint8_t * const  i_wasmBytes,
                                                     uint32_t               i_numWasmBytes);*/
    M3Result  m3_ParseModule  (IM3Environment i_environment, IM3Module * o_module, cbytes_t i_bytes, u32 i_numBytes, IM3Runtime o_runtime);

    // Only modules not loaded into a M3Runtime need to be freed. A module is considered unloaded if
    // a. m3_LoadModule has not yet been called on that module. Or,
    // b. m3_LoadModule returned a result.
    void                m3_FreeModule               (IM3Module i_module);

    //  LoadModule transfers ownership of a module to the runtime. Do not free modules once successfully loaded into the runtime
    M3Result            m3_LoadModule               (IM3Runtime io_runtime,  IM3Module io_module);

    // Optional, compiles all functions in the module
    M3Result            m3_CompileModule            (IM3Module io_module);

    // Calling m3_RunStart is optional
    M3Result            m3_RunStart                 (IM3Module i_module);

    // Arguments and return values are passed in and out through the stack pointer _sp.
    // Placeholder return value slots are first and arguments after. So, the first argument is at _sp [numReturns]
    // Return values should be written into _sp [0] to _sp [num_returns - 1]
    typedef const void * (* M3RawCall) (IM3Runtime runtime, IM3ImportContext _ctx, MEMPTR _sp, IM3Runtime _mem);

    M3Result            m3_LinkRawFunction          (IM3Module              io_module,
                                                     const char * const     i_moduleName,
                                                     const char * const     i_functionName,
                                                     const char * const     i_signature,
                                                     M3RawCall              i_function);

    M3Result            m3_LinkRawFunctionEx        (IM3Module              io_module,
                                                     const char * const     i_moduleName,
                                                     const char * const     i_functionName,
                                                     const char * const     i_signature,
                                                     M3RawCall              i_function,
                                                     const void *           i_userdata);

    const char*         m3_GetModuleName            (IM3Module i_module);
    void                m3_SetModuleName            (IM3Module i_module, const char* name);
    IM3Runtime          m3_GetModuleRuntime         (IM3Module i_module);

//-------------------------------------------------------------------------------------------------------------------------------
//  globals
//-------------------------------------------------------------------------------------------------------------------------------
    IM3Global           m3_FindGlobal               (IM3Module              io_module,
                                                     const char * const     i_globalName);

    M3Result            m3_GetGlobal                (IM3Global              i_global,
                                                     IM3TaggedValue         o_value);

    M3Result            m3_SetGlobal                (IM3Global              i_global,
                                                     const IM3TaggedValue   i_value);

    M3ValueType         m3_GetGlobalType            (IM3Global              i_global);

//-------------------------------------------------------------------------------------------------------------------------------
//  functions
//-------------------------------------------------------------------------------------------------------------------------------
    M3Result            m3_Yield                    (void);

    // o_function is valid during the lifetime of the originating runtime
    M3Result            m3_FindFunction             (IM3Function *          o_function,
                                                     IM3Runtime             i_runtime,
                                                     const char * const     i_functionName);
    M3Result            m3_GetTableFunction         (IM3Function *          o_function,
                                                     IM3Module              i_module,
                                                     uint32_t               i_index);

    uint32_t            m3_GetArgCount              (IM3Function i_function);
    uint32_t            m3_GetRetCount              (IM3Function i_function);
    M3ValueType         m3_GetArgType               (IM3Function i_function, uint32_t i_index);
    M3ValueType         m3_GetRetType               (IM3Function i_function, uint32_t i_index);

    M3Result            m3_CallV                    (IM3Function i_function, ...);
    M3Result            m3_CallVL                   (IM3Function i_function, va_list i_args);
    M3Result            m3_Call                     (IM3Function i_function, uint32_t i_argc, const void * i_argptrs[]);
    M3Result            m3_CallArgv                 (IM3Function i_function, uint32_t i_argc, const char * i_argv[]);

    M3Result            m3_GetResultsV              (IM3Function i_function, ...);
    M3Result            m3_GetResultsVL             (IM3Function i_function, va_list o_rets);
    M3Result            m3_GetResults               (IM3Function i_function, uint32_t i_retc, const void * o_retptrs[]);


    void                m3_GetErrorInfo             (IM3Runtime i_runtime, M3ErrorInfo* o_info);
    void                m3_ResetErrorInfo           (IM3Runtime i_runtime);

    const char*         m3_GetFunctionName          (IM3Function i_function);
    IM3Module           m3_GetFunctionModule        (IM3Function i_function);

//-------------------------------------------------------------------------------------------------------------------------------
//  debug info
//-------------------------------------------------------------------------------------------------------------------------------

    void                m3_PrintRuntimeInfo         (IM3Runtime i_runtime);
    void                m3_PrintM3Info              (void);
    void                m3_PrintProfilerInfo        (void);

    // The runtime owns the backtrace, do not free the backtrace you obtain. Returns NULL if there's no backtrace.
    IM3BacktraceInfo    m3_GetBacktrace             (IM3Runtime i_runtime);

//-------------------------------------------------------------------------------------------------------------------------------
//  raw function definition helpers
//-------------------------------------------------------------------------------------------------------------------------------

/*
# define m3ApiReturnType(TYPE)                 TYPE* raw_return = ((TYPE*) (_sp++));
# define m3ApiMultiValueReturnType(TYPE, NAME) TYPE* NAME = ((TYPE*) (_sp++));
# define m3ApiGetArg(TYPE, NAME)               TYPE NAME = * ((TYPE *) (_sp++));
# define m3ApiGetArgMem(TYPE, NAME)            TYPE NAME = (TYPE)m3ApiOffsetToPtr(* ((uint32_t *) (_sp++)));
# define m3ApiCheckMem(addr, len)   { if (M3_UNLIKELY(((void*)(addr) < _mem) || ((uint64_t)(uintptr_t)(addr) + (len)) > ((uint64_t)(uintptr_t)(_mem)+m3_GetMemorySize(runtime)))) m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess); }
*/

#if WASM_PTRS_64BITS
typedef uint64_t mos; // memory offset
#else
typedef uint32_t mos;
#endif

#define CAST_PTR (mos)(uintptr_t)

typedef void* ptr; //todo: check it

#define m3ApiOffsetToPtr(offset)              m3_ResolvePointer(_mem, offset)
#define m3ApiPtrToOffset(ptr)                 get_offset_pointer(_mem, ptr)

#define m3ApiReturnType(TYPE)                 TYPE* raw_return = ((TYPE*) (m3ApiOffsetToPtr((mos)(uintptr_t)_sp++)));
#define m3ApiMultiValueReturnType(TYPE, NAME) TYPE* NAME = ((TYPE*) (m3ApiOffsetToPtr((mos)(uintptr_t)_sp++)));
#define m3ApiGetArg(TYPE, NAME)               TYPE NAME = *((TYPE *) (m3ApiOffsetToPtr((mos)(uintptr_t)_sp++)));
#define m3ApiGetBaseArg(TYPE, NAME)           TYPE NAME = (TYPE)(*(void**)(m3ApiOffsetToPtr((mos)(uintptr_t)_sp++)));

#define _m3ApiReturnType(TYPE)                 TYPE* raw_return = ((TYPE*) (_sp++));
#define _m3ApiMultiValueReturnType(TYPE, NAME) TYPE* NAME = ((TYPE*) (_sp++));
#define _m3ApiGetArg(TYPE, NAME)               TYPE NAME = * ((TYPE *) (_sp++));
#define _m3ApiGetBaseArg(TYPE, NAME)           TYPE NAME = (TYPE)(*(ptr*)(_sp++));

//#define m3ApiGetArgMem(TYPE, NAME)            TYPE NAME = (TYPE)m3ApiOffsetToPtr((uintptr_t)(* ((uint32_t *) (_sp++)))); 
#define m3ApiGetArgMem(TYPE, NAME)            TYPE NAME = ((TYPE) m3ApiOffsetToPtr(_sp++));
#define m3ApiGetArgArgs(TYPE, NAME, PTR)            TYPE NAME = ((TYPE) m3ApiOffsetToPtr(PTR++));

#define m3ApiTrap(VALUE)                      return VALUE

// Native functions arguments access design
#define m3_GetArgs()            uint64_t* args = (uint64_t*) m3ApiOffsetToPtr(CAST_PTR _sp++); int narg = 0
#define m3_GetReturn(TYPE)      TYPE* raw_return = ((TYPE*) &args[narg++])
#define m3_GetArg(TYPE, NAME)   TYPE NAME = (TYPE) args[narg++]

// todo: implement it with segmentation
#define m3ApiCheckMem(addr, len) 

////////////////////////////////////////////////////////////////

# define m3ApiIsNullPtr(addr)       ((void*)(addr) <= _mem)

# define m3ApiRawFunction(NAME)     const void * NAME (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem)
# define m3ApiReturn(VALUE)                   { *raw_return = (VALUE); return m3Err_none;}
# define m3ApiMultiValueReturn(NAME, VALUE)   { *NAME = (VALUE); }
# define m3ApiTrap(VALUE)                     { return VALUE; }
# define m3ApiSuccess()                       { return m3Err_none; }

# if defined(M3_BIG_ENDIAN)
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        m3_bswap16((* (uint16_t *)(ptr)))
#  define m3ApiReadMem32(ptr)        m3_bswap32((* (uint32_t *)(ptr)))
#  define m3ApiReadMem64(ptr)        m3_bswap64((* (uint64_t *)(ptr)))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr)  = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = m3_bswap16((val)); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = m3_bswap32((val)); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = m3_bswap64((val)); }
# else
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        (* (uint16_t *)(ptr))
#  define m3ApiReadMem32(ptr)        (* (uint32_t *)(ptr))
#  define m3ApiReadMem64(ptr)        (* (uint64_t *)(ptr))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr) = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = (val); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = (val); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = (val); }
# endif

/*
#   define m3ApiReadMem8(ptr)         (*(uint8_t *)m3SegmentedMemAccess(_mem, (uintptr_t)(ptr), sizeof(uint8_t)))
#   define m3ApiReadMem16(ptr)        (*(uint16_t *)m3SegmentedMemAccess(_mem, (uintptr_t)(ptr), sizeof(uint16_t)))
#   define m3ApiReadMem32(ptr)        (*(uint32_t *)m3SegmentedMemAccess(_mem, (uintptr_t)(ptr), sizeof(uint32_t)))
#   define m3ApiReadMem64(ptr)        (*(uint64_t *)m3SegmentedMemAccess(_mem, (uintptr_t)(ptr), sizeof(uint64_t)))
#   define m3ApiWriteMem8(ptr, val)   (*(uint8_t *)m3SegmentedMemAccess(_mem, (uintptr_t)(ptr), sizeof(uint8_t)) = (val))
#   define m3ApiWriteMem16(ptr, val)  (*(uint16_t *)m3SegmentedMemAccess(_mem, (uintptr_t)(ptr), sizeof(uint16_t)) = (val))
#   define m3ApiWriteMem32(ptr, val)  (*(uint32_t *)m3SegmentedMemAccess(_mem, (uintptr_t)(ptr), sizeof(uint32_t)) = (val))
#   define m3ApiWriteMem64(ptr, val)  (*(uint64_t *)m3SegmentedMemAccess(_mem, (uintptr_t)(ptr), sizeof(uint64_t)) = (val))
*/

#if defined(__cplusplus)
}
#endif

///
/// WATCHDOG
///

#define ENABLE_WDT 0

#if ENABLE_WATCHDOG_WASM3 // should read it from the main project
#define ENABLE_WDT 1
#endif

#if ENABLE_WDT
#define CALL_WATCHDOG esp_task_wdt_reset();
//ESP_LOGI("WASM3", "Called task_wdt_reset");
#else 
#define CALL_WATCHDOG
#endif

////
//// Log macro
////
#define LOG_FLUSH ESP_LOGI("WASM3", "flush..."); vTaskDelay(pdMS_TO_TICKS(50))

#include "m3_esp_try.h"

///
/// Native functions
///

#define WASM_NATIVE static M3Result