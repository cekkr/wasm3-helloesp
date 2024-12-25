//
//  m3_function.h
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#pragma once

#include "m3_exception.h"
#include "m3_core.h"

#if PASSTHROUGH_HELLOESP
#include "m3_api_esp_wasi.h"
#endif

d_m3BeginExternC

//---------------------------------------------------------------------------------------------------------------------------------

typedef struct M3FuncType
{
    struct M3FuncType *     next;

    u16                     numRets;
    u16                     numArgs;
    u8                      types [];        // returns, then args
}
M3FuncType;

typedef M3FuncType *        IM3FuncType;


M3Result    AllocFuncType                   (IM3FuncType * o_functionType, u32 i_numTypes);
bool        AreFuncTypesEqual               (const IM3FuncType i_typeA, const IM3FuncType i_typeB);

u16         GetFuncTypeNumParams            (const IM3FuncType i_funcType);
u8          GetFuncTypeParamType            (const IM3FuncType i_funcType, u16 i_index);

u16         GetFuncTypeNumResults           (const IM3FuncType i_funcType);
u8          GetFuncTypeResultType           (const IM3FuncType i_funcType, u16 i_index);

//---------------------------------------------------------------------------------------------------------------------------------

typedef struct M3Function
{
    struct M3Module *       module;

    M3ImportInfo            import;

    bytes_t                 wasm;
    bytes_t                 wasmEnd;

    cstr_t                  names[d_m3MaxDuplicateFunctionImpl];
    cstr_t                  export_name;                            // should be a part of "names"
    u16                     numNames;                               // maximum of d_m3MaxDuplicateFunctionImpl

    IM3FuncType             funcType;

    pc_t                    compiled;

# if (d_m3EnableCodePageRefCounting)
    IM3CodePage *           codePageRefs;                           // array of all pages used
    u32                     numCodePageRefs;
# endif

# if defined (DEBUG)
    u32                     hits;
    u32                     index;
# endif

    u16                     maxStackSlots;

    u16                     numRetSlots;
    u16                     numRetAndArgSlots;

    u16                     numLocals;                              // not including args
    u16                     numLocalBytes;

    bool                    ownsWasmCode;

    u16                     numConstantBytes;
    void *                  constants;
}
M3Function;

typedef M3Function *        IM3Function;

void        Function_Release            (IM3Function i_function);
void        Function_FreeCompiledCode   (IM3Function i_function);

cstr_t      GetFunctionImportModuleName (IM3Function i_function);
cstr_t *    GetFunctionNames            (IM3Function i_function, u16 * o_numNames);
u16         GetFunctionNumArgs          (IM3Function i_function);
u8          GetFunctionArgType          (IM3Function i_function, u32 i_index);

u16         GetFunctionNumReturns       (IM3Function i_function);
u8          GetFunctionReturnType       (const IM3Function i_function, u16 i_index);

u32         GetFunctionNumArgsAndLocals (IM3Function i_function);

cstr_t      SPrintFunctionArgList       (IM3Function i_function, m3stack_t i_sp);

//---------------------------------------------------------------------------------------------------------------------------------

// Function type generation from signature

// Mapping between WebAssembly type characters and WASM3 type values
#define M3_TYPE_NONE    0
#define M3_TYPE_I32     1
#define M3_TYPE_I64     2
#define M3_TYPE_F32     3
#define M3_TYPE_F64     4

M3FuncType* ParseFunctionSignature(const char* signature);


//M3Result addFunctionToModule(IM3Module module, const char* functionName, const char* signature);
M3Result RegisterWasmFunction(IM3Module module, const WasmFunctionEntry* entry, m3_wasi_context_t* ctx);
M3Result RegisterWasmFunctions(IM3Module module, const WasmFunctionEntry* entries, size_t count, m3_wasi_context_t* ctx);


d_m3EndExternC
