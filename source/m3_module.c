//
//  m3_module.c
//
//  Created by Steven Massey on 5/7/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_env.h"
#include "m3_exception.h"
#include "m3_pointers.h"
#include "m3_function.h"

#include "esp_log.h"
#include "esp_debug_helpers.h"

void Module_FreeFunctions (IM3Module i_module)
{
    for (u32 i = 0; i < i_module->numFunctions; ++i)
    {
        IM3Function func = & i_module->functions [i];
        Function_Release (func);
    }
}


void  m3_FreeModule  (IM3Module i_module)
{
    if (i_module)
    {
        m3log (module, "freeing module: %s (funcs: %d; segments: %d)",
               i_module->name, i_module->numFunctions, i_module->numDataSegments);

        Module_FreeFunctions (i_module);

        m3_Int_Free (i_module->functions);
        //m3_Int_Free (i_module->imports);
        m3_Int_Free (i_module->funcTypes);
        m3_Int_Free (i_module->dataSegments);
        m3_Int_Free (i_module->table0);

        for (u32 i = 0; i < i_module->numGlobals; ++i)
        {
            m3_Int_Free (i_module->globals[i].name);
            FreeImportInfo(&(i_module->globals[i].import));
        }
        m3_Int_Free (i_module->globals);
        m3_Int_Free (i_module->memoryExportName);
        m3_Int_Free (i_module->table0ExportName);

        FreeImportInfo(&i_module->memoryImport);

        m3_Int_Free (i_module);
    }
}


M3Result  Module_AddGlobal  (IM3Module io_module, IM3Global * o_global, u8 i_type, bool i_mutable, bool i_isImported)
{
_try {
    u32 index = io_module->numGlobals++;
    io_module->globals = m3_Int_ReallocArray (M3Global, io_module->globals, io_module->numGlobals, index);
    _throwifnull (io_module->globals);
    M3Global * global = & io_module->globals [index];

    global->type = i_type;
    global->imported = i_isImported;
    global->isMutable = i_mutable;

    if (o_global)
        * o_global = global;

} _catch:
    return result;
}

static const bool WASM_DEBUG_PREALLOCFUNCTIONS = true;
static const int PreallocFunctions_PageSize = 8;
M3Result  Module_PreallocFunctions  (IM3Module io_module, u32 i_totalFunctions)
{
    if(WASM_DEBUG_PREALLOCFUNCTIONS && false){
        if(i_totalFunctions > 10000){
            esp_backtrace_print(100);
        }
    }

_try {

    i_totalFunctions = ((i_totalFunctions/PreallocFunctions_PageSize) + 1) * PreallocFunctions_PageSize;

    if(WASM_DEBUG_PREALLOCFUNCTIONS) ESP_LOGI("WASM", "PreallocFunctions: (Total Funcs: %lu, All Funcs: %lu)", i_totalFunctions, io_module->allFunctions);
    if (i_totalFunctions > io_module->allFunctions) {
        if(WASM_DEBUG_PREALLOCFUNCTIONS) ESP_LOGI("WASM", "PreallocFunctions: m3_Int_ReallocArray");

        if(WASM_DEBUG_PREALLOCFUNCTIONS && io_module->functions == NULL){
            ESP_LOGI("WASM", "PreallocFunctions: first time module->functions allocation");
        }

        io_module->functions = m3_Int_ReallocArray (M3Function, io_module->functions, i_totalFunctions, io_module->allFunctions);
        io_module->allFunctions = i_totalFunctions;

        if(WASM_DEBUG_PREALLOCFUNCTIONS) ESP_LOGI("WASM", "PreallocFunctions: allFunctions updated to %lu", io_module->allFunctions);

        if(io_module->functions) return NULL; // ...

        _throwifnull (io_module->functions);
    }
} _catch:
    ESP_LOGE("WASM", "PreallocFunctions error");
    return result;
}

M3Result  Module_AddFunction  (IM3Module io_module, u32 i_typeIndex, IM3ImportInfo i_importInfo)
{
_try {

    u32 index = io_module->numFunctions++;
_   (Module_PreallocFunctions(io_module, io_module->numFunctions));

    _throwif ("type sig index out of bounds", i_typeIndex >= io_module->numFuncTypes);

    IM3FuncType ft = io_module->funcTypes [i_typeIndex];

    IM3Function func = Module_GetFunction (io_module, index);
    
    if(func == NULL){
        func = m3_Int_AllocStruct(M3Function);
    }

    func->funcType = ft;


#   ifdef DEBUG
    func->index = index;
#   endif

    if (i_importInfo and func->numNames == 0)
    {
        func->import = * i_importInfo;
        func->names[0] = i_importInfo->fieldUtf8;
        func->numNames = 1;
    }

    m3log (module, "   added function: %3d; sig: %d", index, i_typeIndex);

} _catch:
    return result;
}

#ifdef DEBUG
void  Module_GenerateNames  (IM3Module i_module)
{
    for (u32 i = 0; i < i_module->numFunctions; ++i)
    {
        IM3Function func = & i_module->functions [i];

        if (func->numNames == 0)
        {
            char* buff = m3_Int_AllocArray(char, 16);
            snprintf(buff, 16, "$func%d", i);
            func->names[0] = buff;
            func->numNames = 1;
        }
    }
    for (u32 i = 0; i < i_module->numGlobals; ++i)
    {
        IM3Global global = & i_module->globals [i];

        if (global->name == NULL)
        {
            char* buff = m3_Int_AllocArray(char, 16);
            snprintf(buff, 16, "$global%d", i);
            global->name = buff;
        }
    }
}
#endif

IM3Function  Module_GetFunction  (IM3Module i_module, u32 i_functionIndex)
{
    IM3Function func = NULL;

    if (i_functionIndex < i_module->numFunctions)
    {
        /*if(is_ptr_valid(& i_module->functions [i_functionIndex]))
        {
            func = & i_module->functions [i_functionIndex];
            //func->module = i_module;
        }
        else {
            ESP_LOGE("WASM", "Module_GetFunction: invalid function pointer");
        }*/

       func = i_module->functions [i_functionIndex];        
    }  
    else {
        ESP_LOGE("WASM", "Module_GetFunction: i_functionIndex >= i_module->numFunctions (%lu, %lu)", i_functionIndex, i_module->numFunctions );
    }  

    return func;
}


const char*  m3_GetModuleName  (IM3Module i_module)
{
    if (!i_module || !i_module->name)
        return ".unnamed";

    return i_module->name;
}

void  m3_SetModuleName  (IM3Module i_module, const char* name)
{
    if (i_module) i_module->name = name;
}

IM3Runtime  m3_GetModuleRuntime  (IM3Module i_module)
{
    return i_module ? i_module->runtime : NULL;
}

