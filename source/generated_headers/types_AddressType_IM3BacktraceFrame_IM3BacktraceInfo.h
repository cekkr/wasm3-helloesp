#pragma once

// Auto-generated header to resolve circular dependencies

#include "m3_code.h"
#include "m3_compile.h"
#include "m3_function.h"
#include "wasm3_defs.h"

class c_m3Type_unknown;
class size_t;
class struct M3BacktraceFrame;
class struct M3Environment;
class struct M3Function;
class struct M3Global;
class struct M3Module;
class struct M3Runtime;
class uint32_t;
class uint64_t;
class uint8_t;

typedef enum {
    ADDRESS_INVALID = 0,
    ADDRESS_STACK,
    ADDRESS_LINEAR
} AddressType;

    struct M3BacktraceFrame *    next;
}
M3BacktraceFrame, * IM3BacktraceFrame;

typedef struct M3BacktraceInfo
{
    IM3BacktraceFrame      frames;

typedef struct M3BacktraceInfo
{
    IM3BacktraceFrame      frames;
    IM3BacktraceFrame      lastFrame;    // can be M3_BACKTRACE_TRUNCATED
}
M3BacktraceInfo, * IM3BacktraceInfo;

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

typedef struct M3ImportContext
{
    void *          userdata;
    IM3Function     function;
}
M3ImportContext, * IM3ImportContext;

typedef struct M3ImportInfo
{
    const char *    moduleUtf8;
    const char *    fieldUtf8;
}
M3ImportInfo, * IM3ImportInfo;

typedef M3Memory *          IM3Memory;

typedef struct M3MemoryPoint_t {
    IM3Memory memory;
    size_t offset;
} M3MemoryPoint;

typedef M3MemoryPoint *          IM3MemoryPoint;

///
/// M3Memory fragmentation
///



////////////////////////////////
IM3Memory m3_NewMemory();
IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem);

void                        InitRuntime                 (IM3Runtime io_runtime, u32 i_stackSizeInBytes);
void                        Runtime_Release             (IM3Runtime io_runtime);
M3Result                    ResizeMemory                (IM3Runtime io_runtime, u32 i_numPages);

bool IsStackAddress(M3Memory* memory, u8* addr);
bool IsLinearAddress(M3Memory* memory, u8* addr);
u8* GetStackAddress(M3Memory* memory, size_t offset);
M3Result GrowStack(M3Memory* memory, size_t additional_size);

// Stack/linear addresses
// Studies: https://claude.ai/chat/699c9c02-0792-40c3-b08e-09b8e5df34c8
M3Result AddSegment(M3Memory* memory);
u8* GetEffectiveAddress(M3Memory* memory, size_t offset);
bool IsStackAddress(M3Memory* memory, u8* addr);
M3Result GrowStack(M3Memory* memory, size_t additional_size);

////////////////////////////////
bool allocate_segment(M3Memory* memory, size_t segment_index);
static inline void* GetMemorySegment(IM3Memory memory, u32 offset);
static inline i32 m3_LoadInt(IM3Memory memory, u32 offset);
static inline void m3_StoreInt(IM3Memory memory, u32 offset, i32 value);

#endif

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

typedef struct M3DataSegment
{
    const u8 *              initExpr;           // wasm code
    const u8 *              data;

    u32                     initExprSize;
    u32                     memoryRegion;
    u32                     size;
}
M3DataSegment;

typedef struct M3Environment
{
//    struct M3Runtime *      runtimes;

    IM3FuncType             funcTypes;                          // linked list of unique M3FuncType structs that can be compared using pointer-equivalence

    IM3FuncType             retFuncTypes [c_m3Type_unknown];    // these 'point' to elements in the linked list above.
                                                                // the number of elements must match the basic types as per M3ValueType
    M3CodePage *            pagesReleased;

    M3SectionHandler        customSectionHandler;
}
M3Environment;

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

typedef struct M3Global
{
    M3ImportInfo            import;

    union
    {
        i32 i32Value;
        i64 i64Value;
#if d_m3HasFloat
        f64 f64Value;
        f32 f32Value;
#endif
    };

    cstr_t                  name;
    bytes_t                 initExpr;       // wasm code
    u32                     initExprSize;
    u8                      type;
    bool                    imported;
    bool                    isMutable;
}
M3Global;

typedef struct M3ImportContext
{
    void *          userdata;
    IM3Function     function;
}
M3ImportContext, * IM3ImportContext;

typedef struct M3ImportInfo
{
    const char *    moduleUtf8;
    const char *    fieldUtf8;
}
M3ImportInfo, * IM3ImportInfo;

typedef struct M3Memory_t {
    //M3MemoryHeader*       mallocated;
    IM3Runtime              runtime;

    //u32                     initPages; // initPages or numPages?
    u32                     numPages;
    u32                     maxPages;
    //u32                     pageSize;

    // Segmentation
    MemorySegment* segments;    // Array di segmenti
    size_t num_segments;        // Current segments number
    size_t segment_size;        // Segment size
    size_t total_size;          // Current total size
    size_t max_size;            // Max size

    // Fragmentation
    M3MemoryRegion stack;      // Regione dello stack (cresce verso il basso)
    M3MemoryRegion linear;     // Regione della memoria lineare (cresce verso l'alto)
    //u8* stack_pointer;         // SP corrente // delegated to M3MemoryPoint

} M3Memory;

typedef struct M3MemoryInfo //todo: study its usage
{
    u32     initPages;
    u32     maxPages;
    u32     pageSize;
}
M3MemoryInfo;

typedef struct M3MemoryPoint_t {
    IM3Memory memory;
    size_t offset;
} M3MemoryPoint;

typedef struct M3MemoryPoint_t {
    IM3Memory memory;
    size_t offset;
} M3MemoryPoint;

typedef struct M3MemoryRegion {
    u8* base;           // Base address of region
    size_t size;        // Current size
    size_t max_size;    // Maximum size
    size_t current_offset; // Current offset within region
} M3MemoryRegion;

typedef struct M3Memory_t {
    //M3MemoryHeader*       mallocated;
    IM3Runtime              runtime;

    //u32                     initPages; // initPages or numPages?
    u32                     numPages;
    u32                     maxPages;
    //u32                     pageSize;

    // Segmentation
    MemorySegment* segments;    // Array di segmenti
    size_t num_segments;        // Current segments number
    size_t segment_size;        // Segment size
    size_t total_size;          // Current total size
    size_t max_size;            // Max size

    // Fragmentation
    M3MemoryRegion stack;      // Regione dello stack (cresce verso il basso)
    M3MemoryRegion linear;     // Regione della memoria lineare (cresce verso l'alto)
    //u8* stack_pointer;         // SP corrente // delegated to M3MemoryPoint

} M3Memory;

typedef struct M3Module
{
    struct M3Runtime *      runtime;
    struct M3Environment *  environment;

    bytes_t                 wasmStart;
    bytes_t                 wasmEnd;

    cstr_t                  name;

    u32                     numFuncTypes;
    IM3FuncType *           funcTypes;              // array of pointers to list of FuncTypes

    u32                     numFuncImports;
    u32                     numFunctions;
    u32                     allFunctions;           // allocated functions count
    M3Function *            functions;

    i32                     startFunction;

    u32                     numDataSegments;
    M3DataSegment *         dataSegments;

    //u32                     importedGlobals;
    u32                     numGlobals;
    M3Global *              globals;

    u32                     numElementSegments;
    bytes_t                 elementSection;
    bytes_t                 elementSectionEnd;

    IM3Function *           table0;
    u32                     table0Size;
    const char*             table0ExportName;

    M3MemoryInfo            memoryInfo;
    M3ImportInfo            memoryImport;
    bool                    memoryImported;
    const char*             memoryExportName;

    //bool                    hasWasmCodeCopy;

    struct M3Module *       next;
}
M3Module;

    typedef const void * (* M3RawCall) (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem);

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

# define m3ApiOffsetToPtr(offset)   (void*)((uint8_t*)_mem + (uint32_t)(offset))
# define m3ApiPtrToOffset(ptr)      (uint32_t)((uint8_t*)ptr - (uint8_t*)_mem)

# define m3ApiReturnType(TYPE)                 TYPE* raw_return = ((TYPE*) (_sp++));
# define m3ApiMultiValueReturnType(TYPE, NAME) TYPE* NAME = ((TYPE*) (_sp++));
# define m3ApiGetArg(TYPE, NAME)               TYPE NAME = * ((TYPE *) (_sp++));
# define m3ApiGetArgMem(TYPE, NAME)            TYPE NAME = (TYPE)m3ApiOffsetToPtr(* ((uint32_t *) (_sp++)));

# define m3ApiIsNullPtr(addr)       ((void*)(addr) <= _mem)
# define m3ApiCheckMem(addr, len)   { if (M3_UNLIKELY(((void*)(addr) < _mem) || ((uint64_t)(uintptr_t)(addr) + (len)) > ((uint64_t)(uintptr_t)(_mem)+m3_GetMemorySize(runtime)))) m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess); }

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

typedef struct M3Runtime
{
    M3Compilation *           compilation;

    IM3Environment          environment;

    M3CodePage *            pagesOpen;      // linked list of code pages with writable space on them
    M3CodePage *            pagesFull;      // linked list of at-capacity pages

    u32                     numCodePages;
    u32                     numActiveCodePages;

    IM3Module               modules;        // linked list of imported modules

    /* // Original stack management
    void *                  stack;
    void *                  originStack;
    u32                     stackSize;
    u32                     numStackSlots;
    */

    IM3Memory               stack;
    IM3Memory               originStack;
    u32                     maxStackSize;

    IM3Function             lastCalled;     // last function that successfully executed

    void *                  userdata;

    M3Memory                memory;
    u32                     memoryLimit;

#if d_m3EnableStrace >= 2
    u32                     callDepth;
#endif

    M3ErrorInfo             error;
#if d_m3VerboseErrorMessages
    char                    error_message[256]; // the actual buffer. M3ErrorInfo can point to this
#endif

#if d_m3RecordBacktraces
    M3BacktraceInfo         backtrace;
#endif

	u32						newCodePageSequence;
}
M3Runtime;

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
    M3Result            m3_ParseModule              (IM3Environment         i_environment,
                                                     IM3Module *            o_module,
                                                     const uint8_t * const  i_wasmBytes,
                                                     uint32_t               i_numWasmBytes);

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
    typedef const void * (* M3RawCall) (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem);

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

# define m3ApiOffsetToPtr(offset)   (void*)((uint8_t*)_mem + (uint32_t)(offset))
# define m3ApiPtrToOffset(ptr)      (uint32_t)((uint8_t*)ptr - (uint8_t*)_mem)

# define m3ApiReturnType(TYPE)                 TYPE* raw_return = ((TYPE*) (_sp++));
# define m3ApiMultiValueReturnType(TYPE, NAME) TYPE* NAME = ((TYPE*) (_sp++));
# define m3ApiGetArg(TYPE, NAME)               TYPE NAME = * ((TYPE *) (_sp++));
# define m3ApiGetArgMem(TYPE, NAME)            TYPE NAME = (TYPE)m3ApiOffsetToPtr(* ((uint32_t *) (_sp++)));

# define m3ApiIsNullPtr(addr)       ((void*)(addr) <= _mem)
# define m3ApiCheckMem(addr, len)   { if (M3_UNLIKELY(((void*)(addr) < _mem) || ((uint64_t)(uintptr_t)(addr) + (len)) > ((uint64_t)(uintptr_t)(_mem)+m3_GetMemorySize(runtime)))) m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess); }

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

typedef enum M3ValueType
{
    c_m3Type_none   = 0,
    c_m3Type_i32    = 1,
    c_m3Type_i64    = 2,
    c_m3Type_f32    = 3,
    c_m3Type_f64    = 4,

    c_m3Type_unknown
} M3ValueType;

typedef struct MemorySegment {
    void* data;           // Actual data pointer
    bool is_allocated;    // Allocation flag
    size_t stack_size;    // Current stack size in this segment
    size_t linear_size;   // Current linear memory size in this segment
} MemorySegment;

typedef void *              (* ModuleVisitor)           (IM3Module i_module, void * i_info);
void *                      ForEachModule               (IM3Runtime i_runtime, ModuleVisitor i_visitor, void * i_info);

void *                      v_FindFunction              (IM3Module i_module, const char * const i_name);

IM3CodePage                 AcquireCodePage             (IM3Runtime io_runtime);
IM3CodePage                 AcquireCodePageWithCapacity (IM3Runtime io_runtime, u32 i_lineCount);
void                        ReleaseCodePage             (IM3Runtime io_runtime, IM3CodePage i_codePage);

d_m3EndExternC

#endif // m3_env_h
