//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

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
    bool a_valid = ultra_safe_ptr_valid(i_typeA);
    bool b_valid = ultra_safe_ptr_valid(i_typeB);
    if(!a_valid || !b_valid){        
        //ESP_LOGW("WASM3", "Invalid pointer for AreFuncTypesEqual");
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
/// Function signature
///

static const bool WASM_DEBUG_PARSE_FUNCTION_SIGNATURE = true;

M3FuncType* ParseFunctionSignature(const char* signature) {
    if (!signature) {
        ESP_LOGW("WASM3", "ParseFunctionSignature: returns NULL (null signature)");
        return NULL;
    }

    if(WASM_DEBUG_PARSE_FUNCTION_SIGNATURE) ESP_LOGI("WASM3", "ParseFunctionSignature called with signature: %s", signature);
    
    size_t len = strlen(signature);
    u16 numRets = 0;
    u16 numArgs = 0;
    size_t i = 0;
    
    // Process return type (before the parenthesis)
    if (signature[i] == 'v') {
        numRets = 0;
    } else {
        switch (signature[i]) {
            case 'i':
                numRets = 1;
                break;
            case 'I':
                numRets = 1;
                break;
            case 'f':
                numRets = 1;
                break;
            case 'F':
                numRets = 1;
                break;
            default:
                ESP_LOGW("WASM3", "ParseFunctionSignature: Invalid return type %c", signature[i]);
                return NULL;
        }
    }
    
    // Find opening parenthesis
    while (i < len && signature[i] != '(') i++;
    if (i >= len) {
        ESP_LOGW("WASM3", "ParseFunctionSignature: Missing opening parenthesis");
        return NULL;
    }
    i++; // Skip '('
    
    // Count arguments until ')'
    size_t args_start = i;
    while (i < len && signature[i] != ')') {
        if (signature[i] != ' ') numArgs++;
        i++;
    }
    
    if (i >= len || signature[i] != ')') {
        ESP_LOGW("WASM3", "ParseFunctionSignature: Missing closing parenthesis");
        return NULL;
    }
    
    // Allocate memory for the structure plus the types array
    size_t totalSize = sizeof(M3FuncType) + (numRets + numArgs) * sizeof(u8);
    M3FuncType* funcType = (M3FuncType*)default_allocator.malloc(totalSize);
    if (!funcType) {
        ESP_LOGW("WASM3", "ParseFunctionSignature: Memory allocation failed");
        return NULL;
    }
    
    // Initialize the structure
    funcType->next = NULL;
    funcType->numRets = numRets;
    funcType->numArgs = numArgs;
    
    // Fill the types array
    u8* types = funcType->types;
    size_t typeIndex = 0;
    
    // First process return type (if any)
    if (numRets > 0) {
        switch (signature[0]) {
            case 'i':
                types[typeIndex++] = M3_TYPE_I32;
                break;
            case 'I':
                types[typeIndex++] = M3_TYPE_I64;
                break;
            case 'f':
                types[typeIndex++] = M3_TYPE_F32;
                break;
            case 'F':
                types[typeIndex++] = M3_TYPE_F64;
                break;
            default:
                types[typeIndex++] = M3_TYPE_NONE;
                break;
        }
    }
    
    // Process arguments
    for (i = args_start; signature[i] != ')'; i++) {
        if (signature[i] != ' ') {
            switch (signature[i]) {
                case 'i':
                    types[typeIndex++] = M3_TYPE_I32;
                    break;
                case 'I':
                    types[typeIndex++] = M3_TYPE_I64;
                    break;
                case 'f':
                    types[typeIndex++] = M3_TYPE_F32;
                    break;
                case 'F':
                    types[typeIndex++] = M3_TYPE_F64;
                    break;
                default:
                    ESP_LOGE("WASM3", "ParseFunctionSignature: Unknown argument type %c at position %d", signature[i], i);
                    default_allocator.free(funcType);
                    return NULL;
            }
        }
    }

    if (WASM_DEBUG_PARSE_FUNCTION_SIGNATURE) {
        ESP_LOGI("WASM3", "ParseFunctionSignature: Successfully parsed signature '%s' (returns: %d, args: %d)", 
                 signature, numRets, numArgs);
    }
    
    return funcType;
}

// Utility function to free the M3FuncType structure
void FreeFuncType(M3FuncType* funcType) {
    default_allocator.free(funcType);
}

///
/// Register function name
///

static const bool WASM_DEBUG_ADD_FUNCTION_NAME = true;
M3Result addFunctionToModule(IM3Module module, const char* functionName, const char* signature) {
    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "addFunctionToModule called");

    if (!module || !functionName) {
        return "Invalid parameters";
    }

    // Alloca spazio per il nuovo nome della funzione
    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "m3_Int_AllocArray function name");
    char* nameCopy = m3_Int_AllocArray(char, strlen(functionName) + 1);
    if (!nameCopy) {
        return "nameCopy memory allocation failed";
    }
    strcpy(nameCopy, functionName);

    // Crea una nuova entry nella function table   
    u32 index = module->numFunctions++;
    //ESP_LOGI("WASM3", "index: %lu, allFunctions: %lu", index, module->allFunctions); // just debug

    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "Module_PreallocFunctions");
    Module_PreallocFunctions(module, module->numFunctions);

    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "Module_GetFunction");
    IM3Function function = Module_GetFunction (module, index);

    if (!function) {
        ESP_LOGE("WASM3", "addFunctionToModule: Module_GetFunction failed");
        return "Module_GetFunction failed";
    }
    
    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "Setting function parameters");
    function->import.fieldUtf8 = nameCopy;
    function->import.moduleUtf8 = module->name; //strdup(module->name);  
    function->names[0] = function->import.moduleUtf8;
    function->numNames++;
    function->module = module;
    function->compiled = NULL;  // Non compilata
    function->funcType = ParseFunctionSignature(signature);  // Tipo non definito    

    return m3Err_none;
}

// Funzione helper per registrare una funzione nel modulo
M3Result RegisterWasmFunction(IM3Module module, const WasmFunctionEntry* entry) {
    M3Result result = m3Err_none;
    
    if (!module || !entry || !entry->name || !entry->func) {
        return "Invalid parameters";
    }

    // Verifica dei parametri con log
    if (!module || !entry || !entry->name || !entry->func) {
        ESP_LOGE("WASM", "Invalid parameters - module: %p, entry: %p", 
                 (void*)module, (void*)entry);
        return "Invalid parameters";
    }

    addFunctionToModule(module, entry->name, entry->signature);

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

