//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_env.h"
#include "m3_exception.h"
#include "m3_info.h"
#include "m3_pointers.h"

#include "esp_log.h"

u8  ConvertTypeCharToTypeId (char i_code)
{
    switch (i_code) {
    case 'v': return c_m3Type_none;
    case 'i': return c_m3Type_i32;
    case 'I': return c_m3Type_i64;
    case 'f': return c_m3Type_f32;
    case 'F': return c_m3Type_f64;
    case '*': return c_m3Type_i32;
    }
    return c_m3Type_unknown;
}


M3Result  SignatureToFuncType  (IM3FuncType * o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = NULL;

_try {
    if (not o_functionType)
        _throw ("null function type");

    if (not i_signature)
        _throw ("null function signature");

    cstr_t sig = i_signature;

    size_t maxNumTypes = strlen (i_signature);

    // assume min signature is "()"
    _throwif (m3Err_malformedFunctionSignature, maxNumTypes < 2);
    maxNumTypes -= 2;

    _throwif (m3Err_tooManyArgsRets, maxNumTypes > d_m3MaxSaneFunctionArgRetCount);

_   (AllocFuncType (& funcType, (u32) maxNumTypes));

    u8 * typelist = funcType->types;

    bool parsingRets = true;
    while (* sig)
    {
        char typeChar = * sig++;

        if (typeChar == '(')
        {
            parsingRets = false;
            continue;
        }
        else if ( typeChar == ' ')
            continue;
        else if (typeChar == ')')
            break;

        u8 type = ConvertTypeCharToTypeId (typeChar);

        _throwif ("unknown argument type char", c_m3Type_unknown == type);

        if (type == c_m3Type_none)
            continue;

        if (parsingRets)
        {
            _throwif ("malformed signature; return count overflow", funcType->numRets >= maxNumTypes);
            funcType->numRets++;
            *typelist++ = type;
        }
        else
        {
            _throwif ("malformed signature; arg count overflow", (u32)(funcType->numRets) + funcType->numArgs >= maxNumTypes);
            funcType->numArgs++;
            *typelist++ = type;
        }
    }

} _catch:

    if (result)
        m3_Int_Free (funcType);

    * o_functionType = funcType;

    return result;
}

////////////////////////////////////////////////////////////////
// Log signature

// Convertiamo da type ID a char
char convertTypeIdToChar(u8 typeId) {
    switch (typeId) {
        case c_m3Type_i32: return 'i';
        case c_m3Type_i64: return 'I';
        case c_m3Type_f32: return 'f';
        case c_m3Type_f64: return 'F';
        default: return '?';
    }
}

char* m3_type_to_signature(const M3FuncType* type) {
    if (!type) return NULL;
    
    // Calcoliamo la dimensione necessaria per la stringa
    size_t size = 3 + type->numArgs + type->numRets;  // v() + types + \0
    char* signature = (char*)malloc(size);
    if (!signature) return NULL;
    
    char* curr = signature;
    
    // Return type (sempre 'v' nel tuo caso dato che hai mostrato 'v' nell'output)
    *curr++ = 'v';
    
    // Parentesi aperta
    *curr++ = '(';
    
    // Tipi degli argomenti
    for (int i = 0; i < type->numArgs; i++) {
        *curr++ = convertTypeIdToChar(type->types[type->numRets + i]);
    }
    
    // Parentesi chiusa e terminatore
    *curr++ = ')';
    *curr = '\0';
    
    return signature;
}

////////////////////////////////////////////////////////////////

static
M3Result  ValidateSignature  (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = NULL;
_   (SignatureToFuncType (& ftype, i_linkingSignature));

    if (not AreFuncTypesEqual (ftype, i_function->funcType))
    {
        m3log (module, "expected: %s", SPrintFuncTypeSignature (ftype));
        m3log (module, "   found: %s", SPrintFuncTypeSignature (i_function->funcType));
        
        ESP_LOGE("WASM3", "Signature excepted: %s", m3_type_to_signature (ftype));
        ESP_LOGE("WASM3", "Signature found: %s", m3_type_to_signature (i_function->funcType));

        _throw ("function signature mismatch");
    }

    _catch:

    m3_Int_Free (ftype);

    return result;
}


M3Result  FindAndLinkFunction      (IM3Module       io_module,
                                    ccstr_t         i_moduleName,
                                    ccstr_t         i_functionName,
                                    ccstr_t         i_signature,
                                    voidptr_t       i_function,
                                    voidptr_t       i_userdata)
{
_try {
    _throwif(m3Err_moduleNotLinked, !io_module->runtime);

    const bool wildcardModule = (strcmp (i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    for (u32 i = 0; i < io_module->numFunctions; ++i)
    {
        const IM3Function f = & io_module->functions [i];

        if (f->import.moduleUtf8 and f->import.fieldUtf8)
        {
            if (strcmp (f->import.fieldUtf8, i_functionName) == 0 and
               (wildcardModule or strcmp (f->import.moduleUtf8, i_moduleName) == 0))
            {
                if (i_signature) {
_                   (ValidateSignature (f, i_signature));
                }
_               (CompileRawFunction (io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    return result;
}

M3Result  m3_LinkRawFunctionEx  (IM3Module            io_module,
                                const char * const    i_moduleName,
                                const char * const    i_functionName,
                                const char * const    i_signature,
                                M3RawCall             i_function,
                                const void *          i_userdata)
{
    return FindAndLinkFunction (io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result  m3_LinkRawFunction  (IM3Module            io_module,
                              const char * const    i_moduleName,
                              const char * const    i_functionName,
                              const char * const    i_signature,
                              M3RawCall             i_function)
{
    return FindAndLinkFunction (io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, NULL);
}

