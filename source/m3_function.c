//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#include "m3_function.h"
#include "m3_env.h"

#include "m3_pointers.h"

M3Result AllocFuncType (IM3FuncType * o_functionType, u32 i_numTypes)
{
    *o_functionType = (IM3FuncType) m3_Int_Malloc ("M3FuncType", sizeof (M3FuncType) + i_numTypes); 

    if(!(*o_functionType)){
        ESP_LOGE("WASM3", "AllocFuncType allocation failed");
    }

    return (*o_functionType) ? m3Err_none : m3Err_mallocFailed;
}


/*bool  AreFuncTypesEqual  (const IM3FuncType i_typeA, const IM3FuncType i_typeB)
{
    if (i_typeA->numRets == i_typeB->numRets && i_typeA->numArgs == i_typeB->numArgs)
    {
        return (memcmp (i_typeA->types, i_typeB->types, i_typeA->numRets + i_typeA->numArgs) == 0);
    }

    return false;
}*/

bool AreFuncTypesEqual(const IM3FuncType i_typeA, const IM3FuncType i_typeB)
{
    if(!ultra_safe_ptr_valid(i_typeA) || !ultra_safe_ptr_valid(i_typeB)){
        ESP_LOGE("WASM3", "Invalid pointer for AreFuncTypesEqual");
        return false;
    }

    // Validazione puntatori
    //if (!i_typeA || !i_typeB)
    //    return false;
        
    if (i_typeA->numRets == i_typeB->numRets && i_typeA->numArgs == i_typeB->numArgs)
    {
        size_t size = i_typeA->numRets + i_typeA->numArgs;
        
        // Confronto byte per byte invece di memcmp
        for (size_t i = 0; i < size; i++) {
            if (i_typeA->types[i] != i_typeB->types[i])
                return false;
        }
        return true;
    }

    return false;
}

u16  GetFuncTypeNumParams  (const IM3FuncType i_funcType)
{
    return i_funcType ? i_funcType->numArgs : 0;
}


u8  GetFuncTypeParamType  (const IM3FuncType i_funcType, u16 i_index)
{
    u8 type = c_m3Type_unknown;

    if (i_funcType)
    {
        if (i_index < i_funcType->numArgs)
        {
            type = i_funcType->types [i_funcType->numRets + i_index];
        }
    }

    return type;
}



u16  GetFuncTypeNumResults  (const IM3FuncType i_funcType)
{
    return i_funcType ? i_funcType->numRets : 0;
}


u8  GetFuncTypeResultType  (const IM3FuncType i_funcType, u16 i_index)
{
    u8 type = c_m3Type_unknown;

    if (i_funcType)
    {
        if (i_index < i_funcType->numRets)
        {
            type = i_funcType->types [i_index];
        }
    }

    return type;
}


//---------------------------------------------------------------------------------------------------------------


void FreeImportInfo (M3ImportInfo * i_info)
{
    m3_Int_Free (i_info->moduleUtf8);
    m3_Int_Free (i_info->fieldUtf8);
}

static const bool WASM_DEBUG_FUNCTION_RELEASE = false;

void  Function_Release  (IM3Function i_function)
{    
    if(WASM_DEBUG_FUNCTION_RELEASE) ESP_LOGI("WASM3", "Function_Release called");
    //m3_Int_Free (i_function->constants);

    safe_m3_int_free((void**)&(i_function->constants));

    for (int i = 0; i < i_function->numNames; i++)
    {
        if(WASM_DEBUG_FUNCTION_RELEASE) ESP_LOGI("WASM3", "freeing i_function->numNames[%d]", i);
        // name can be an alias of fieldUtf8
        if (i_function->names[i] != i_function->import.fieldUtf8)
        {
            safe_m3_int_free((void**)&(i_function->names[i]));
        }
    }

    if(WASM_DEBUG_FUNCTION_RELEASE) ESP_LOGI("WASM3", "FreeImportInfo(..)");
    FreeImportInfo (& i_function->import);

    if (i_function->ownsWasmCode){
        if(WASM_DEBUG_FUNCTION_RELEASE) ESP_LOGI("WASM3", "free i_function->wasm");
        safe_m3_int_free((void**)&(i_function->wasm));
    }

    // Function_FreeCompiledCode (func);

#   if (d_m3EnableCodePageRefCounting)
    {
        m3_Int_Free (i_function->codePageRefs);
        i_function->numCodePageRefs = 0;
    }

   
#   endif

    safe_m3_int_free((void**)&(i_function));
}


void  Function_FreeCompiledCode (IM3Function i_function)
{
#   if (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = NULL;

        while (i_function->numCodePageRefs--)
        {
            IM3CodePage page = i_function->codePageRefs [i_function->numCodePageRefs];

            if (--(page->info.usageCount) == 0)
            {
//                printf ("free %p\n", page);
            }
        }

        m3_Int_Free (i_function->codePageRefs);

        Runtime_ReleaseCodePages (i_function->module->runtime);
    }
#   endif
}


cstr_t  m3_GetFunctionName  (IM3Function i_function)
{
    u16 numNames = 0;
    cstr_t *names = GetFunctionNames(i_function, &numNames);
    if (numNames > 0)
        return names[0];
    else
        return "<unnamed>";
}


IM3Module  m3_GetFunctionModule  (IM3Function i_function)
{
    return i_function ? i_function->module : NULL;
}


cstr_t *  GetFunctionNames  (IM3Function i_function, u16 * o_numNames)
{
    if (!i_function || !o_numNames)
        return NULL;

    if (i_function->import.fieldUtf8)
    {
        *o_numNames = 1;
        return &i_function->import.fieldUtf8;
    }
    else
    {
        *o_numNames = i_function->numNames;
        return i_function->names;
    }
}


cstr_t  GetFunctionImportModuleName  (IM3Function i_function)
{
    return (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16  GetFunctionNumArgs  (IM3Function i_function)
{
    u16 numArgs = 0;

    if (i_function)
    {
        if (i_function->funcType)
            numArgs = i_function->funcType->numArgs;
    }

    return numArgs;
}

u8  GetFunctionArgType  (IM3Function i_function, u32 i_index)
{
    u8 type = c_m3Type_none;

    if (i_index < GetFunctionNumArgs (i_function))
    {
        u32 numReturns = i_function->funcType->numRets;

        type = i_function->funcType->types [numReturns + i_index];
    }

    return type;
}


u16  GetFunctionNumReturns  (IM3Function i_function)
{
    u16 numReturns = 0;

    if (i_function)
    {
        if (i_function->funcType)
            numReturns = i_function->funcType->numRets;
    }

    return numReturns;
}


u8  GetFunctionReturnType  (const IM3Function i_function, u16 i_index)
{
    return i_function ? GetFuncTypeResultType (i_function->funcType, i_index) : c_m3Type_unknown;
}


u32  GetFunctionNumArgsAndLocals (IM3Function i_function)
{
    if (i_function)
        return i_function->numLocals + GetFunctionNumArgs (i_function);
    else
        return 0;
}

///
/// Register function name
///

static const bool WASM_DEBUG_ADD_FUNCTION_NAME = true;
M3Result addFunctionToModule(IM3Module module, const char* functionName) {
    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "addFunctionToModule called");

    if (!module || !functionName) {
        return "Invalid parameters";
    }

    // Alloca spazio per il nuovo nome della funzione
    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "m3_Int_AllocArray function name");
    char* nameCopy = m3_Int_AllocArray(char, strlen(functionName) + 1);
    if (!nameCopy) {
        return "Memory allocation failed";
    }
    strcpy(nameCopy, functionName);

    // Crea una nuova entry nella function table   
    u32 index = module->allFunctions;
    module->allFunctions++;
    //ESP_LOGI("WASM3", "index: %lu, allFunctions: %lu", index, module->allFunctions); // just debug

    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "Module_PreallocFunctions");
    Module_PreallocFunctions(module, module->allFunctions + 1);

    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "Module_GetFunction");
    IM3Function function = Module_GetFunction (module, index);

    if (!function) {
        ESP_LOGE("WASM3", "addFunctionToModule: Module_GetFunction failed");
        return "Module_GetFunction failed";
    }
    
    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "Setting function parameters");
    function->import.fieldUtf8 = nameCopy;
    function->import.moduleUtf8 = strdup(module->name);  
    function->names[0] = function->import.moduleUtf8;
    function->numNames++;
    function->module = module;
    function->compiled = NULL;  // Non compilata
    function->funcType = NULL;  // Tipo non definito    

    return m3Err_none;
}

// Funzione helper per registrare una funzione nel modulo
M3Result RegisterWasmFunction(IM3Module module, const WasmFunctionEntry* entry) {
    M3Result result = m3Err_none;
    
    if (!module || !entry || !entry->name || !entry->func) {
        return "Invalid parameters";
    }

    addFunctionToModule(module, entry->name);
    
    // Linkare la funzione nel modulo
    result = m3_LinkRawFunction(
        module,              // Modulo WASM
        "*",                 // Namespace (wildcard)
        entry->name,         // Nome della funzione
        entry->signature,    // Firma della funzione
        entry->func         // Puntatore alla funzione
    );
    
    return result;
}

// Funzione per registrare multiple funzioni da un array
M3Result RegisterWasmFunctions(IM3Module module, const WasmFunctionEntry* entries, size_t count) {
    M3Result result = m3Err_none;
    
    for (size_t i = 0; i < count; i++) {
        result = RegisterWasmFunction(module, &entries[i]);
        ESP_LOGI("WASM3", "Registered native function: %s\n", entries[i].name);
        if (result) {
            return result; // Ritorna al primo errore
        }
    }
    
    return result;
}

// Esempio di utilizzo:
/*
// Definizione delle funzioni native
m3_ret_t add(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t* _sp, void* _mem) {
    // Implementazione della funzione
    return m3Err_none;
}

// Creazione del lookup table
const WasmFunctionEntry functionTable[] = {
    { "add", add, "i(ii)" },  // Funzione che accetta due interi e ritorna un intero
    // Aggiungi altre funzioni qui...
};

// Registrazione delle funzioni
M3Result result = RegisterWasmFunctions(module, functionTable, sizeof(functionTable)/sizeof(functionTable[0]));
*/

