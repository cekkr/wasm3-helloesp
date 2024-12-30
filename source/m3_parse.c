//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_parse.h"

DEBUG_TYPE WASM_DEBUG_PARSE = WASM_DEBUG_ALL || (WASM_DEBUG && false);

M3Result  ParseType_Table  (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    return result;
}


const bool WASM_FORCE_MAX_PAGE_SIZE = false;
DEBUG_TYPE WASM_DEBUG_PARSETYPE_MEMORY = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  ParseType_Memory  (M3MemoryInfo * o_memory, bytes_t * io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u8 flag;

_   (ReadLEB_u7 (o_memory->mem, & flag, io_bytes, i_end));                   // really a u1
_   (ReadLEB_u32 (o_memory->mem, & o_memory->initPages, io_bytes, i_end));

    o_memory->maxPages = 0;
    if (flag & (1u << 0))
_       (ReadLEB_u32 (o_memory->mem, & o_memory->maxPages, io_bytes, i_end));

    o_memory->pageSize = 0;
    if (flag & (1u << 3)) {
        u32 logPageSize;
_       (ReadLEB_u32 (o_memory->mem, & logPageSize, io_bytes, i_end));
        o_memory->pageSize = 1u << logPageSize;
    }

    if(WASM_DEBUG_PARSETYPE_MEMORY){
        ESP_LOGI("WASM3", "ParseType_Memory: o_memory->initPages = %d", o_memory->initPages);
        ESP_LOGI("WASM3", "ParseType_Memory: o_memory->maxPages = %d", o_memory->maxPages);
        ESP_LOGI("WASM3", "ParseType_Memory: o_memory->pageSize = %d", o_memory->pageSize);
    }
    
    if(WASM_FORCE_MAX_PAGE_SIZE){
        //o_memory->pageSize = 1024*1024*4; // 4 GB
        o_memory->pageSize = 1024; 
    }

    _catch: return result;
}


M3Result  ParseSection_Type  (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = NULL;
    IM3Memory mem = &io_module->runtime->memory;

_try {
    u32 numTypes;
_   (ReadLEB_u32 (mem, & numTypes, & i_bytes, i_end));                                   m3log (parse, "** Type [%d]", numTypes);

    _throwif("too many types", numTypes > d_m3MaxSaneTypesCount);

    if (numTypes)
    {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_Def_AllocArray (IM3FuncType, numTypes);
        _throwifnull (io_module->funcTypes);
        io_module->numFuncTypes += numTypes;

        for (u32 i = 0; i < numTypes; ++i)
        {
            i8 form;
_           (ReadLEB_i7 (mem, & form, & i_bytes, i_end));
            _throwif (m3Err_wasmMalformed, form != -32); // for Wasm MVP

            u32 numArgs;
_           (ReadLEB_u32 (mem, & numArgs, & i_bytes, i_end));

            _throwif (m3Err_tooManyArgsRets, numArgs > d_m3MaxSaneFunctionArgRetCount);
#if defined(M3_COMPILER_MSVC)
            u8 argTypes [d_m3MaxSaneFunctionArgRetCount];
#else
            u8 argTypes[numArgs+1]; // make ubsan happy
#endif
            for (u32 a = 0; a < numArgs; ++a)
            {
                i8 wasmType;
                u8 argType;
_               (ReadLEB_i7 (mem, & wasmType, & i_bytes, i_end));
_               (NormalizeType (& argType, wasmType));

                argTypes[a] = argType;
            }

            u32 numRets;
_           (ReadLEB_u32 (mem, & numRets, & i_bytes, i_end));
            _throwif (m3Err_tooManyArgsRets, (u64)(numRets) + numArgs > d_m3MaxSaneFunctionArgRetCount);

_           (AllocFuncType (& ftype, numRets + numArgs));
            ftype->numArgs = numArgs;
            ftype->numRets = numRets;

            for (u32 r = 0; r < numRets; ++r)
            {
                i8 wasmType;
                u8 retType;
_               (ReadLEB_i7 (mem, & wasmType, & i_bytes, i_end));
_               (NormalizeType (& retType, wasmType));

                ftype->types[r] = retType;
            }
            memcpy (ftype->types + numRets, argTypes, numArgs);                                 m3log (parse, "    type %2d: %s", i, SPrintFuncTypeSignature (ftype));

            Environment_AddFuncType (io_module->environment, & ftype);
            io_module->funcTypes [i] = ftype;
            ftype = NULL; // ownership transferred to environment
        }
    }

} _catch:

    if (result)
    {
        m3_Def_Free (ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Def_Free (io_module->funcTypes);
        io_module->numFuncTypes = 0;
    }

    return result;
}


DEBUG_TYPE WASM_DEBUG_ParseSection_Function = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  ParseSection_Function  (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;
    IM3Memory mem = &io_module->runtime->memory;

    if(mem == NULL || mem->firm != INIT_FIRM){
        if(WASM_DEBUG_ParseSection_Function) ESP_LOGW("WASM3", "ParseSection_Function: mem not initialized");
    }

    u32 numFunctions;
_   (ReadLEB_u32 (mem, & numFunctions, & i_bytes, i_end));                               m3log (parse, "** Function [%d]", numFunctions);

    if(WASM_DEBUG_ParseSection_Function) ESP_LOGW("WASM3", "ParseSection_Function: numFunctions: %d", numFunctions);    

    _throwif("too many functions", numFunctions > d_m3MaxSaneFunctionsCount);

_   (Module_PreallocFunctions(io_module, io_module->numFunctions + numFunctions));

    for (u32 i = 0; i < numFunctions; ++i)
    {
        u32 funcTypeIndex;
_       (ReadLEB_u32 (mem, & funcTypeIndex, & i_bytes, i_end));

_       (Module_AddFunction (io_module, funcTypeIndex, NULL /* import info */));
    }

    _catch: return result;
}


DEBUG_TYPE WASM_DEBUG_PARSE_SECTION = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  ParseSection_Import  (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;
    IM3Memory mem = &io_module->runtime->memory;

    M3ImportInfo import = { 0 }, clearImport = { 0 };

    u32 numImports;
_   (ReadLEB_u32 (mem, & numImports, & i_bytes, i_end));                                 m3log (parse, "** Import [%d]", numImports);

    _throwif("too many imports", numImports > d_m3MaxSaneImportsCount);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, numImports));

    for (u32 i = 0; i < numImports; ++i)
    {
        u8 importKind;

_       (Read_utf8 (mem, & import.moduleUtf8, & i_bytes, i_end));
_       (Read_utf8 (mem, & import.fieldUtf8, & i_bytes, i_end));
_       (Read_u8 (mem, & importKind, & i_bytes, i_end));                                 m3log (parse, "    kind: %d '%s.%s' ",
                                                                                                (u32) importKind, import.moduleUtf8, import.fieldUtf8);
        switch (importKind)
        {
            case d_externalKind_function:
            {
                u32 typeIndex;
_               (ReadLEB_u32 (mem, & typeIndex, & i_bytes, i_end))

_               (Module_AddFunction (io_module, typeIndex, & import))
                import = clearImport;

                io_module->numFuncImports++;
            }
            break;

            case d_externalKind_table:
//                  result = ParseType_Table (& i_bytes, i_end);
                break;

            case d_externalKind_memory:
            {
_               (ParseType_Memory (& io_module->memoryInfo, & i_bytes, i_end));
                io_module->memoryImported = true;
                io_module->memoryImport = import;
                import = clearImport;
            }
            break;

            case d_externalKind_global:
            {
                i8 waType;
                u8 type, isMutable;

_               (ReadLEB_i7 (mem, & waType, & i_bytes, i_end));
_               (NormalizeType (& type, waType));
_               (ReadLEB_u7 (mem, & isMutable, & i_bytes, i_end));                     m3log (parse, "     global: %s mutable=%d", c_waTypes [type], (u32) isMutable);

                IM3Global global;
_               (Module_AddGlobal (io_module, & global, type, isMutable, true /* isImport */));
                global->import = import;
                import = clearImport;
            }
            break;

            default:
                _throw (m3Err_wasmMalformed);
        }

        FreeImportInfo (& import);
    }

    _catch:

    FreeImportInfo (& import);

    return result;
}


M3Result  ParseSection_Export  (IM3Module io_module, bytes_t i_bytes, cbytes_t  i_end)
{
    M3Result result = m3Err_none;
    IM3Memory mem = &io_module->runtime->memory;
    const char * utf8 = NULL;

    u32 numExports;
_   (ReadLEB_u32 (mem, & numExports, & i_bytes, i_end));                                 m3log (parse, "** Export [%d]", numExports);

    _throwif("too many exports", numExports > d_m3MaxSaneExportsCount);

    for (u32 i = 0; i < numExports; ++i)
    {
        u8 exportKind;
        u32 index;

_       (Read_utf8 (mem, & utf8, & i_bytes, i_end));
_       (Read_u8 (mem, & exportKind, & i_bytes, i_end));
_       (ReadLEB_u32 (mem, & index, & i_bytes, i_end));                                  m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) exportKind, utf8);

        if (exportKind == d_externalKind_function)
        {
            _throwif(m3Err_wasmMalformed, index >= io_module->numFunctions);
            IM3Function func = &(io_module->functions [index]);
            if (func->numNames < d_m3MaxDuplicateFunctionImpl)
            {
                func->names[func->numNames++] = utf8;
                func->export_name = utf8;
                utf8 = NULL; // ownership transferred to M3Function
            }
        }
        else if (exportKind == d_externalKind_global)
        {
            _throwif(m3Err_wasmMalformed, index >= io_module->numGlobals);
            IM3Global global = &(io_module->globals [index]);
            m3_Def_Free (global->name);
            global->name = utf8;
            utf8 = NULL; // ownership transferred to M3Global
        }
        else if (exportKind == d_externalKind_memory)
        {
            m3_Def_Free (io_module->memoryExportName);
            io_module->memoryExportName = utf8;
            utf8 = NULL; // ownership transferred to M3Module
        }
        else if (exportKind == d_externalKind_table)
        {
            m3_Def_Free (io_module->table0ExportName);
            io_module->table0ExportName = utf8;
            utf8 = NULL; // ownership transferred to M3Module
        }

        m3_Def_Free (utf8);
    }

_catch:
    m3_Def_Free (utf8);
    return result;
}


M3Result  ParseSection_Start  (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 startFuncIndex;
_   (ReadLEB_u32 (&io_module->runtime->memory, & startFuncIndex, & i_bytes, i_end));                               m3log (parse, "** Start Function: %d", startFuncIndex);

    if (startFuncIndex < io_module->numFunctions)
    {
        io_module->startFunction = startFuncIndex;
    }
    else result = "start function index out of bounds";

    _catch: return result;
}


M3Result  Parse_InitExpr  (M3Module * io_module, bytes_t * io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    CHECK_MEMORY_PTR(&io_module->runtime->memory, "ParseSection_Global");

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#if defined(d_m3PreferStaticAlloc)
    static M3Compilation compilation;
#else
    M3Compilation compilation;
#endif
    compilation = (M3Compilation){ .runtime = io_module->runtime, .module = io_module, .wasm = * io_bytes, .wasmEnd = i_end };

    result = CompileBlockStatements (& compilation);

    * io_bytes = compilation.wasm;

    return result;
}


M3Result  ParseSection_Element  (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 numSegments;
_   (ReadLEB_u32 (&io_module->runtime->memory, & numSegments, & i_bytes, i_end));                         m3log (parse, "** Element [%d]", numSegments);

    _throwif ("too many element segments", numSegments > d_m3MaxSaneElementSegments);

    io_module->elementSection = i_bytes;
    io_module->elementSectionEnd = i_end;
    io_module->numElementSegments = numSegments;

    _catch: return result;
}


M3Result  ParseSection_Code  (M3Module * io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result;
    IM3Memory mem = &io_module->runtime->memory;

    u32 numFunctions;
_   (ReadLEB_u32 (mem, & numFunctions, & i_bytes, i_end));                               m3log (parse, "** Code [%d]", numFunctions);

    if (numFunctions != io_module->numFunctions - io_module->numFuncImports)
    {
        _throw ("mismatched function count in code section");
    }

    for (u32 f = 0; f < numFunctions; ++f)
    {
        const u8 * start = i_bytes;

        u32 size;
_       (ReadLEB_u32 (mem, & size, & i_bytes, i_end));

        if (size)
        {
            const u8 * ptr = i_bytes;
            i_bytes += size;

            if (i_bytes <= i_end)
            {
                /*
                u32 numLocalBlocks;
_               (ReadLEB_u32 (& numLocalBlocks, & ptr, i_end));                                      m3log (parse, "    code size: %-4d", size);

                u32 numLocals = 0;

                for (u32 l = 0; l < numLocalBlocks; ++l)
                {
                    u32 varCount;
                    i8 wasmType;
                    u8 normalType;

_                   (ReadLEB_u32 (& varCount, & ptr, i_end));
_                   (ReadLEB_i7 (& wasmType, & ptr, i_end));
_                   (NormalizeType (& normalType, wasmType));

                    numLocals += varCount;                                                      m3log (parse, "      %2d locals; type: '%s'", varCount, c_waTypes [normalType]);
                }
                 */

                IM3Function func = Module_GetFunction (io_module, f + io_module->numFuncImports);

                func->module = io_module;
                func->wasm = start;
                func->wasmEnd = i_bytes;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
//                func->numLocals = numLocals;
            }
            else _throw (m3Err_wasmSectionOverrun);
        }
    }

    _catch:

    if (not result and i_bytes != i_end)
        result = m3Err_wasmSectionUnderrun;

    return result;
}

DEBUG_TYPE WASM_DEBUG_PARSESECTION_DATA = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  ParseSection_Data  (M3Module * io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;
    IM3Memory mem = &io_module->runtime->memory;

    u32 numDataSegments;
_   (ReadLEB_u32 (mem, & numDataSegments, & i_bytes, i_end));                            m3log (parse, "** Data [%d]", numDataSegments);

    _throwif("too many data segments", numDataSegments > d_m3MaxSaneDataSegments);

    io_module->dataSegments = m3_Def_AllocArray (M3DataSegment, numDataSegments);
    _throwifnull(io_module->dataSegments);
    io_module->numDataSegments = numDataSegments;

    if(WASM_DEBUG_PARSESECTION_DATA) ESP_LOGI("WASM3", "ParseSection_Data: numDataSegments = %d", numDataSegments);

    for (u32 i = 0; i < numDataSegments; ++i)
    {
        M3DataSegment * segment = & io_module->dataSegments [i];

_       (ReadLEB_u32 (mem, & segment->memoryRegion, & i_bytes, i_end));

        segment->initExpr = i_bytes;
_       (Parse_InitExpr (io_module, & i_bytes, i_end));
        segment->initExprSize = (mos) (i_bytes - segment->initExpr);

        _throwif (m3Err_wasmMissingInitExpr, segment->initExprSize <= 1);

_       (ReadLEB_u32 (mem, & segment->size, & i_bytes, i_end));
        segment->data = i_bytes;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->memoryRegion, segment->initExprSize, segment->size);
        i_bytes += segment->size;

        if(WASM_DEBUG_PARSESECTION_DATA) {
            ESP_LOGI("WASM3", "ParseSection_Data: segment: %d, initExpr: %p, initExprSize: %d, size: %d, i_bytes: %p, i_end: %p", 
            i, segment->initExpr, segment->initExprSize, segment->size, i_bytes, i_end);
            ESP_LOGI("WASM3", "ParseSection_Data: segment->memoryRegion = %d", segment->memoryRegion);
        }

        _throwif("data segment underflow", i_bytes > i_end);
    }

    _catch:

    return result;
}


DEBUG_TYPE WASM_DEBUG_PARSESECTION_MEMORY = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  ParseSection_Memory  (M3Module * io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // TODO: MVP; assert no memory imported

    u32 numMemories;
_   (ReadLEB_u32 (&io_module->runtime->memory, & numMemories, & i_bytes, i_end));                             m3log (parse, "** Memory [%d]", numMemories);

    if(WASM_DEBUG_PARSESECTION_MEMORY) ESP_LOGI("WASM3", "ParseSection_Memory: numMemories = %d", numMemories);

    _throwif (m3Err_tooManyMemorySections, numMemories != 1);

    ParseType_Memory (& io_module->memoryInfo, & i_bytes, i_end);

    _catch: return result;
}


M3Result  ParseSection_Global  (M3Module * io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;
    IM3Memory mem = &io_module->runtime->memory;

    CHECK_MEMORY_PTR(mem, "ParseSection_Global");

    u32 numGlobals;
_   (ReadLEB_u32 (mem, & numGlobals, & i_bytes, i_end));                                 m3log (parse, "** Global [%d]", numGlobals);

    _throwif("too many globals", numGlobals > d_m3MaxSaneGlobalsCount);

    for (u32 i = 0; i < numGlobals; ++i)
    {
        i8 waType;
        u8 type, isMutable;

_       (ReadLEB_i7 (mem, & waType, & i_bytes, i_end));
_       (NormalizeType (& type, waType));
_       (ReadLEB_u7 (mem, & isMutable, & i_bytes, i_end));                                 m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [type],   (u32) isMutable);

        IM3Global global;
_       (Module_AddGlobal (io_module, & global, type, isMutable, false /* isImport */));

        global->initExpr = i_bytes;
_       (Parse_InitExpr (io_module, & i_bytes, i_end));
        global->initExprSize = (u32) (i_bytes - global->initExpr);

        _throwif (m3Err_wasmMissingInitExpr, global->initExprSize <= 1);
    }

    _catch: return result;
}


M3Result  ParseSection_Name  (M3Module * io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result;
    IM3Memory mem = &io_module->runtime->memory;

    cstr_t name;

    while (i_bytes < i_end)
    {
        u8 nameType;
        u32 payloadLength;

_       (ReadLEB_u7 (mem, & nameType, & i_bytes, i_end));
_       (ReadLEB_u32 (mem, & payloadLength, & i_bytes, i_end));

        bytes_t start = i_bytes;
        if (nameType == 1)
        {
            u32 numNames;
_           (ReadLEB_u32 (mem, & numNames, & i_bytes, i_end));

            _throwif("too many names", numNames > d_m3MaxSaneFunctionsCount);

            for (u32 i = 0; i < numNames; ++i)
            {
                u32 index;
_               (ReadLEB_u32 (mem, & index, & i_bytes, i_end));
_               (Read_utf8 (mem, & name, & i_bytes, i_end));

                if (index < io_module->numFunctions)
                {
                    IM3Function func = &(io_module->functions [index]);
                    if (func->numNames == 0)
                    {
                        func->names[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->numNames = 1;
                        name = NULL; // transfer ownership
                    }
//                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Def_Free (name);
            }
        }

        i_bytes = start + payloadLength;
    }

    _catch: return result;
}


M3Result  ParseSection_Custom  (M3Module * io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result;

    cstr_t name;
_   (Read_utf8 (&io_module->runtime->memory, & name, & i_bytes, i_end));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    if (strcmp (name, "name") == 0) {
_       (ParseSection_Name(io_module, i_bytes, i_end));
    } else if (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_bytes, i_end));
    }

    m3_Def_Free (name);

    _catch: return result;
}


DEBUG_TYPE WASM_DEBUG_PARSEMODULESECTION = WASM_DEBUG_ALL || (WASM_DEBUG && false);
DEBUG_TYPE WASM_DEBUG_PARSEMODULESECTION_FUNCTION = WASM_DEBUG_ALL || (WASM_DEBUG && false); // activate backtrace on function section

M3Result  ParseModuleSection  (M3Module * o_module, u8 i_sectionType, bytes_t i_bytes, u32 i_numBytes)
{
    if(WASM_DEBUG_PARSEMODULESECTION){
        ESP_LOGI("WASM3", "ParseModuleSection called");
        ESP_LOGI("WASM3", "ParseModuleSection: i_sectionType: %d", i_sectionType);
        ESP_LOGI("WASM3", "ParseModuleSection: i_bytes: %p", i_bytes);
        ESP_LOGI("WASM3", "ParseModuleSection: i_numBytes: %d", i_numBytes);
    }

    M3Result result = m3Err_none;

    CHECK_MEMORY_PTR(&o_module->runtime->memory, "ParseModuleSection");

    typedef M3Result (* M3Parser) (M3Module *, bytes_t, cbytes_t);

    static M3Parser s_parsers [] =
    {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        NULL,                   // 4: TODO Table
        ParseSection_Memory,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        ParseSection_Start,     // 8
        ParseSection_Element,   // 9
        ParseSection_Code,      // 10
        ParseSection_Data,      // 11
        NULL,                   // 12: TODO DataCount
    };

    M3Parser parser = NULL;

    if (i_sectionType <= 12)
        parser = s_parsers [i_sectionType];

    if (parser)
    {
        cbytes_t end = i_bytes + i_numBytes;

        if(WASM_DEBUG_PARSEMODULESECTION_FUNCTION && i_sectionType == 3){
            ESP_LOGI("WASM3", "ParseModuleSection: Parsing Function Section");
            backtrace();
        }

        result = parser (o_module, i_bytes, end);
    }
    else
    {
        m3log (parse, " skipped section type: %d", (u32) i_sectionType);
    }

    return result;
}

#define WASM_ParseModule_EndWithExceptedSection true

DEBUG_TYPE WASM_DEBUG_PARSE_MODULE = WASM_DEBUG_ALL || (WASM_DEBUG && false);
static const bool WASM_PARSE_MODULE_IGNORE_SECTION_ORDER = false;
DEBUG_TYPE WASM_DEBUG_PARSE_MODULE_EXCEPTED_SECTION = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  m3_ParseModule  (IM3Environment i_environment, IM3Module * o_module, cbytes_t i_bytes, u32 i_numBytes, IM3Runtime o_runtime)
{
    IM3Module module;          
                                                                 m3log (parse, "load module: %d bytes", i_numBytes);
_try {
    if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule start");
    module = m3_Def_AllocStruct(M3Module);
    if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: module allocated");
    _throwifnull (module);

    IM3Memory mem = NULL;
    if(o_runtime != NULL){
        if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: assigning given runtime and its memory");
        module->runtime = o_runtime;

        if(module->runtime->memory.firm != INIT_FIRM){
            ESP_LOGE("WASM3", "m3_ParseModule: given runtime's memory is not initialized");
            return m3Err_runtimeMemoryNotInit;
        }

        mem = &module->runtime->memory;
        module->memoryInfo.mem = mem;
    }
    else {
        ESP_LOGW("WASM3", "m3_ParseModule: module lacks of runtime and memory");
        M3Runtime nullRuntime = {0};
        nullRuntime.memory.firm = DUMMY_MEMORY_FIRM;
        module->runtime = & nullRuntime;
    }    
    
    CHECK_MEMORY_PTR(mem, "m3_ParseModule mem");
    CHECK_MEMORY_PTR(&module->runtime->memory, "m3_ParseModule &module->runtime->memory");

    module->name = ".unnamed";                                                      m3log (parse, "load module: %d bytes", i_numBytes);
    module->startFunction = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;    

    if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: assignation dones.");

    const u8 * pos = i_bytes;
    const u8 * end = pos + i_numBytes;

    if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: calculated pos: %p, i_numBytes: %p, calculated end: %p\n", pos, i_numBytes, end);

    module->wasmStart = pos;
    module->wasmEnd = end;

    u32 magic, version;
_   (Read_u32 (mem, & magic, & pos, end));
_   (Read_u32 (mem, & version, & pos, end));

    if(WASM_DEBUG_PARSE_MODULE){
        ESP_LOGI("WASM3", "m3_ParseModule: magic: %x (excepted 0x6d736100)", magic);
        ESP_LOGI("WASM3", "m3_ParseModule: version: %x", version);
    }

    _throwif (m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif (m3Err_incompatibleWasmVersion, version != 1);

    static const u8 sectionsOrder[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
    u8 expectedSection = 0;

    if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: cycling");
    while (pos < end)
    {
        const u8 * cyclePos = pos;

        if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: while pos(%p) < end(%p)", pos, end);

        if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: cycle: ReadLEB_u7");
        u8 section;
_       (ReadLEB_u7 (mem, & section, & pos, end));

        if(WASM_DEBUG_PARSE_MODULE){
            ESP_LOGI("WASM3", "Reading section type: %d, current offset: %p", section, i_bytes);
            //ESP_LOGI("WASM3", "Section header bytes: %02x %02x %02x %02x", i_bytes[0], i_bytes[1], i_bytes[2], i_bytes[3]);
        }

        if (section != 0 && !WASM_PARSE_MODULE_IGNORE_SECTION_ORDER) {
            // Ensure sections appear only once and in order
            bool forcedEnd = false;
            while (sectionsOrder[expectedSection++] != section) {
                if(WASM_DEBUG_PARSE_MODULE) ESP_LOGD("WASM3", "Expected section order: %d, found: %d", sectionsOrder[expectedSection-1], section);

                if(expectedSection >= 12){
                    if(WASM_ParseModule_EndWithExceptedSection){
                        ESP_LOGW("WASM3", "m3_ParseModule: forced end cycle");
                        forcedEnd = true;
                        break;
                    }
                    else {
                        ESP_LOGE("WASM3", "m3_ParseModule: WASM section (%d) not found on not in order", section);   

                        if(WASM_DEBUG_PARSE_MODULE_EXCEPTED_SECTION){    
                            LOG_FLUSH;
                            backtrace();             
                        }

                        _throwif(m3Err_misorderedWasmSection, expectedSection >= 12);
                    }
                }               
            }

            if(forcedEnd){
                end = cyclePos;
                module->wasmEnd = cyclePos;
                break;
            }
        }

        if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: cycle: ReadLEB_u32");
        u32 sectionLength;
_       (ReadLEB_u32 (mem, & sectionLength, & pos, end));
        _throwif(m3Err_wasmMalformed, pos + sectionLength > end);

        if(WASM_DEBUG_PARSE_MODULE) ESP_LOGI("WASM3", "m3_ParseModule: cycle: ParseModuleSection (pos: %d, sectionLength: %d)", pos, sectionLength);
_       (ParseModuleSection (module, section, pos, sectionLength));

        pos += sectionLength;
    }

    if(mem != NULL && mem->firm == INIT_FIRM){
        u32 pages = module->memoryInfo.maxPages;
        if(pages < module->memoryInfo.initPages) pages = module->memoryInfo.initPages;
        u32 req_size =  pages * module->memoryInfo.pageSize;

        if(WASM_DEBUG_PARSE_MODULE) ESP_LOGW("WASM3", "m3_ParseModule: module requested size: %d", req_size);

        mem->total_requested_size += req_size;
    }

} _catch:

    if (result)
    {
        if(WASM_DEBUG_PARSE){
            const char* error_type;
            if (result == m3Err_wasmMalformed) {
                error_type = "WASM malformato";
            } else if (result == m3Err_incompatibleWasmVersion) {
                error_type = "Versione WASM incompatibile";
            } else if (result == m3Err_misorderedWasmSection) {
                error_type = "Sezioni WASM in ordine errato";
            } else {
                error_type = "Errore sconosciuto";
            }
            
            ESP_LOGE("WASM3", "Errore nel parsing del modulo: %s (%s)", result, error_type);
        }

        m3_FreeModule (module);
        module = NULL;
    }

    * o_module = module;

    return result;
}
