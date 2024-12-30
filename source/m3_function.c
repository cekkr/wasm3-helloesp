//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "m3_function.h"
//#include "m3_env.h"
#include "m3_bind.h" 
#include "wasm3.h"
//#include "m3_pointers.h"

M3Result AllocFuncType (IM3FuncType * o_functionType, u32 i_numTypes)
{
    *o_functionType = (IM3FuncType) m3_Def_Malloc (sizeof (M3FuncType) + i_numTypes); //was + i_numTypes (why??)

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
    m3_Def_Free (i_info->moduleUtf8);
    m3_Def_Free (i_info->fieldUtf8);
}


DEBUG_TYPE WASM_DEBUG_FUNCTION_RELEASE = WASM_DEBUG_ALL || (WASM_DEBUG && false);
void  Function_Release  (IM3Function i_function)
{    
    #define FUNCTIONS_ON_SEGMENTED_MEM 0

    if(WASM_DEBUG_FUNCTION_RELEASE) ESP_LOGI("WASM3", "Function_Release called");

    #if FUNCTIONS_ON_SEGMENTED_MEM
    m3_Dyn_Free(&i_function->module->runtime->memory, i_function->constants);
    #else
    m3_Def_Free(i_function->constants);
    #endif 

    for (int i = 0; i < i_function->numNames; i++)
    {
        if(WASM_DEBUG_FUNCTION_RELEASE) ESP_LOGI("WASM3", "freeing i_function->numNames[%d]", i);
        // name can be an alias of fieldUtf8
        if (i_function->names[i] != i_function->import.fieldUtf8)
        {
            #if FUNCTIONS_ON_SEGMENTED_MEM
            m3_Dyn_Free(&i_function->module->runtime->memory, i_function->names[i]);
            #else
            m3_Def_Free(i_function->names[i]);
            #endif
        }
    }

    if(WASM_DEBUG_FUNCTION_RELEASE) ESP_LOGI("WASM3", "FreeImportInfo(..)");
    FreeImportInfo (& i_function->import);

    if (i_function->ownsWasmCode){
        if(WASM_DEBUG_FUNCTION_RELEASE) ESP_LOGI("WASM3", "free i_function->wasm");
        #if FUNCTIONS_ON_SEGMENTED_MEM
        m3_Dyn_Free(&i_function->module->runtime->memory, i_function->wasm);
        #else
        m3_Def_Free(i_function->wasm);
        #endif
    }

    // Function_FreeCompiledCode (func);

#   if (d_m3EnableCodePageRefCounting)
    {
        #if FUNCTIONS_ON_SEGMENTED_MEM
        m3_Dyn_Free(i_function->module->runtime->memory, i_function->codePageRefs);
        #else
        m3_Def_Free(i_function->codePageRefs);
        #endif

        i_function->numCodePageRefs = 0;
    }

   
#   endif

    // This is redundant, functions are allocated in groups of functions
    /*#if FUNCTIONS_ON_SEGMENTED_MEM
    m3_Dyn_Free(&i_function->module->runtime->memory, i_function);
    #else
    m3_Def_Free(i_function);
    #endif*/
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

        m3_Def_Free (i_function->codePageRefs);

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

DEBUG_TYPE WASM_DEBUG_PARSE_FUNCTION_SIGNATURE = WASM_DEBUG_ALL || (WASM_DEBUG && false);

// (Probably) abandoned function (SignatureToFuncType from WASM3)
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
    //size_t totalSize = sizeof(M3FuncType) + ((numRets + numArgs) * sizeof(u8));
    IM3FuncType funcType = NULL; // (IM3FuncType)m3_Def_Malloc("M3FuncType", totalSize); // or m3_Def_AllocStruct(M3FuncType)

    M3Result result;
    if((result = AllocFuncType(&funcType, numRets + numArgs)) != NULL){
        ESP_LOGE("WASM3", "ParseFunctionSignature: AllocFuncType error: %s", result);
        return NULL;
    }

    /*if (!funcType) {
        ESP_LOGW("WASM3", "ParseFunctionSignature: Memory allocation failed");
        return NULL;
    }*/
    
    // Initialize the structure
    funcType->next = NULL;
    funcType->numRets = numRets;
    funcType->numArgs = numArgs;
    
    // Fill the types array
    u8* types = funcType->types;
    
    // First process return type (if any)
    if (numRets > 0) {
        switch (signature[0]) {
            case 'i':
                *types = M3_TYPE_I32;
                break;
            case 'I':
                *types = M3_TYPE_I64;
                break;
            case 'f':
                *types = M3_TYPE_F32;
                break;
            case 'F':
                *types = M3_TYPE_F64;
                break;
            default:
                ESP_LOGE("WASM3", "ParseFunctionSignature: Invalid return type: %c", signature[i]);
                m3_Def_Free(funcType);
                return NULL;
        }
    }

    types++;
    
    // Process arguments
    for (i = args_start; signature[i] != ')'; i++) {
        if (signature[i] != ' ') {
            switch (signature[i]) {
                case 'i':
                    *types = M3_TYPE_I32;
                    break;
                case 'I':
                    *types = M3_TYPE_I64;
                    break;
                case 'f':
                    *types = M3_TYPE_F32;
                    break;
                case 'F':
                    *types = M3_TYPE_F64;
                    break;
                default:
                    ESP_LOGE("WASM3", "ParseFunctionSignature: Unknown argument type %c at position %d", signature[i], i);
                    m3_Def_Free(funcType);
                    return NULL;
            }

            types++;
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

DEBUG_TYPE WASM_DEBUG_ADD_FUNCTION_NAME = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result addFunctionToModule(IM3Module module, const char* functionName, const char* signature) {
    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "addFunctionToModule called");

    if (!module || !functionName) {
        return "Invalid parameters";
    }

    // Alloca spazio per il nuovo nome della funzione
    if(WASM_DEBUG_ADD_FUNCTION_NAME) ESP_LOGI("WASM3", "m3_Def_AllocArray function name %lu %s", strlen(functionName), functionName);
    char* nameCopy = m3_Def_AllocArray(char, strlen(functionName) + 1);
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

    //function->funcType = ParseFunctionSignature(signature);  // Tipo non definito    
    M3Result signatureResult = SignatureToFuncType(& function->funcType, signature);
    if(signatureResult){
        ESP_LOGE("WASM3", "addFunctionToModule: SignatureToFuncType failed: %s", signatureResult);
    }    

    return m3Err_none;
}

// Funzione helper per registrare una funzione nel modulo
DEBUG_TYPE WASM_DEBUG_RegisterWasmFunction = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result RegisterWasmFunction(IM3Module module, const WasmFunctionEntry* entry, m3_wasi_context_t* ctx) {
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

    ///
    /// Convert p to pointers size
    ///
    size_t slen = strlen(entry->signature);
    char* signature = malloc(sizeof(char)*(slen+1));
    signature[slen] = '\0'; // is this necessary (with the slen+1)
    strcpy(signature, entry->signature);

    for(int s=0; s<slen; s++){
        if(signature[s]=='p'){
            if(WASM_PTRS_64BITS)
                signature[s] = 'I';
            else 
                signature[s] = 'i';
        }
    }

    if(WASM_DEBUG_RegisterWasmFunction) ESP_LOGI("WASM3", "New signature for %s: %s", entry->name, signature);

    /// 
    ///
    ///
    addFunctionToModule(module, entry->name, signature);

    // Linkare la funzione nel modulo
    result = m3_LinkRawFunctionEx( 
        module,              // Modulo WASM
        "env",                 // Namespace (wildcard)
        entry->name,         // Nome della funzione
        signature,    // Firma della funzione
        entry->func,         // Puntatore alla funzione
        ctx
    );

    if(WASM_DEBUG_RegisterWasmFunction) ESP_LOGI("WASM3", "Function %s registered", entry->name);
    return result;
}

// Funzione per registrare multiple funzioni da un array
DEBUG_TYPE WASM_DEBUG_RegisterWasmFunctions = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result RegisterWasmFunctions(IM3Module module, const WasmFunctionEntry* entries, size_t count, m3_wasi_context_t* ctx) {
    M3Result result = m3Err_none;
    
    for (size_t i = 0; i < count; i++) {
        result = RegisterWasmFunction(module, &entries[i], ctx);
        if(WASM_DEBUG_RegisterWasmFunctions) ESP_LOGI("WASM3", "Registered native function: %s\n", entries[i].name);
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

