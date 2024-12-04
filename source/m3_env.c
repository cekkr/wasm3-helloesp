//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include <stdarg.h>
#include <limits.h>

#include "m3_env.h"
#include "m3_compile.h"
#include "m3_exception.h"
#include "m3_info.h"
#include "m3_pointers.h"

#define _DEBUG_MEMORY 1

////////////////////////////////////////////////////////////////
// Segmented memory management

bool allocate_segment(M3Memory* memory, size_t segment_index) {
    if (!memory || segment_index >= memory->num_segments) {
        return false;
    }

    // Se il segmento è già allocato, restituisci true
    if (memory->segments[segment_index].is_allocated) {
        return true;
    }

    if (_DEBUG_MEMORY) {
        ESP_LOGI("WASM3", "Allocating segment %zu of size %zu", 
                 segment_index, memory->segment_size);
    }

    // Prima prova ad allocare in SPIRAM se abilitata
    void* ptr = WASM_ENABLE_SPI_MEM ? 
                heap_caps_malloc(memory->segment_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM) : 
                NULL;

    // Se l'allocazione in SPIRAM fallisce o non è disponibile, prova la memoria interna
    if (!ptr) {
        ptr = heap_caps_malloc(memory->segment_size, 
                             MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        
        if (_DEBUG_MEMORY && !ptr) {
            ESP_LOGE("WASM3", "Failed to allocate segment in internal memory");
        }
    }
    else if (_DEBUG_MEMORY) {
        ESP_LOGI("WASM3", "Segment allocated in SPIRAM");
    }

    if (ptr) {
        // Inizializza il segmento con zeri
        memset(ptr, 0, memory->segment_size);
        
        memory->segments[segment_index].data = ptr;
        memory->segments[segment_index].is_allocated = true;
        
        if (_DEBUG_MEMORY) {
            ESP_LOGI("WASM3", "Segment %zu successfully allocated", segment_index);
        }
        
        return true;
    }

    return false;
}

static inline void* GetMemorySegment(IM3Memory memory, u32 offset)
{
    size_t segment_index = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    MemorySegment segment = memory->segments[segment_index];
    return (u8*)segment.data + segment_offset;
}

static inline i32 m3_LoadInt(IM3Memory memory, u32 offset)
{
    void* ptr = GetMemorySegment(memory, offset);
    return *(i32*)ptr;
}

static inline void m3_StoreInt(IM3Memory memory, u32 offset, i32 value)
{
    void* ptr = GetMemorySegment(memory, offset);
    *(i32*)ptr = value;
}

////////////////////////////////////////////////////////////////

static const bool WASM_DEBUG_NEW_ENV = false;

IM3Environment  m3_NewEnvironment  ()
{
    if(WASM_DEBUG_NEW_ENV) ESP_LOGI("WASM3", "m3_NewEnvironment called");
    IM3Environment env = m3_Int_AllocStruct (M3Environment);
    if(WASM_DEBUG_NEW_ENV) ESP_LOGI("WASM3", "env allocated");

    if (env)
    {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes
            for (u8 t = c_m3Type_none; t <= c_m3Type_f64; t++)
            {
                IM3FuncType ftype;
_               (AllocFuncType (& ftype, 1));

                ftype->numArgs = 0;
                ftype->numRets = (t == c_m3Type_none) ? 0 : 1;
                ftype->types [0] = t;

                Environment_AddFuncType (env, & ftype);

                d_m3Assert (t < 5);
                env->retFuncTypes [t] = ftype;

                if(WASM_DEBUG_NEW_ENV) ESP_LOGI("WASM3", "Ended an Environment_AddFuncType");
            }
        }

        if(WASM_DEBUG_NEW_ENV) ESP_LOGI("WASM3", "m3_NewEnvironment done.");

        _catch:
        if (result)
        {
            m3_FreeEnvironment (env);
            env = NULL;
        }
    }

    return env;
}


void  Environment_Release  (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    while (ftype)
    {
        IM3FuncType next = ftype->next;
        m3_Int_Free (ftype);
        ftype = next;
    }

    m3log (runtime, "freeing %d pages from environment", CountCodePages (i_environment->pagesReleased));
    FreeCodePages (& i_environment->pagesReleased);
}


void  m3_FreeEnvironment  (IM3Environment i_environment)
{
    if (i_environment)
    {
        Environment_Release (i_environment);
        m3_Int_Free (i_environment);
    }
}


void m3_SetCustomSectionHandler  (IM3Environment i_environment, M3SectionHandler i_handler)
{
    if (i_environment) i_environment->customSectionHandler = i_handler;
}


static const bool WASM_DEBUG_ADDFUNC = false;

// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
void  Environment_AddFuncType  (IM3Environment i_environment, IM3FuncType * io_funcType)
{
    if(WASM_DEBUG_ADDFUNC) ESP_LOGI("WASM3", "Called Environment_AddFuncType");

    IM3FuncType addType = * io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    while (newType)
    {
        if (AreFuncTypesEqual (newType, addType))
        {
            m3_Int_Free (addType);
            break;
        }
        else {
            if(!ultra_safe_ptr_valid(addType)){
                ESP_LOGE("WASM3", "Invalid addType pointer in Environment_AddFuncType");
                return;
            }

            if(!ultra_safe_ptr_valid(newType)){
                ESP_LOGE("WASM3", "Invalid newType pointer in Environment_AddFuncType");
                m3_Int_Free (addType);
                return;
            }
        }

        newType = newType->next;
    }

    if (newType == NULL)
    {
        newType = addType;
        newType->next = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    * io_funcType = newType;

    if(WASM_DEBUG_ADDFUNC) ESP_LOGI("WASM3", "End of Environment_AddFuncType call");
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage ** io_list, u32 i_minimumLineCount)
{
    IM3CodePage prev = NULL;
    IM3CodePage page = * io_list;

    while (page)
    {
        if (NumFreeLines (page) >= i_minimumLineCount)
        {                                                           d_m3Assert (page->info.usageCount == 0);
            IM3CodePage next = page->info.next;
            if (prev)
                prev->info.next = next; // mid-list
            else
                * io_list = next;       // front of list

            break;
        }

        prev = page;
        page = page->info.next;
    }

    return page;
}


IM3CodePage  Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimumLineCount)
{
    return RemoveCodePageOfCapacity (& i_environment->pagesReleased, i_minimumLineCount);
}


void  Environment_ReleaseCodePages  (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    while (end)
    {
        end->info.lineIndex = 0; // reset page
#if d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        if (not next)
            break;

        end = next;
    }

    if (end)
    {
        // push list to front
        end->info.next = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime  m3_NewRuntime  (IM3Environment i_environment, u32 i_stackSizeInBytes, void * i_userdata)
{
    ESP_LOGI("WASM3", "m3_NewRuntime called");

    IM3Runtime runtime = m3_Int_AllocStruct (M3Runtime);

    if (runtime)
    {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata = i_userdata;

        runtime->originStack = m3_Int_Malloc ("Wasm Stack", i_stackSizeInBytes + 4*sizeof (m3slot_t)); // TODO: more precise stack checks

        if (runtime->originStack)
        {
            runtime->stack = runtime->originStack;
            runtime->numStackSlots = i_stackSizeInBytes / sizeof (m3slot_t);         
            m3log (runtime, "new stack: %p", runtime->originStack);
        }
        else m3_Int_Free (runtime);
    }

    return runtime;
}

void *  m3_GetUserData  (IM3Runtime i_runtime)
{
    return i_runtime ? i_runtime->userdata : NULL;
}


void *  ForEachModule  (IM3Runtime i_runtime, ModuleVisitor i_visitor, void * i_info)
{
    void * r = NULL;

    IM3Module module = i_runtime->modules;

    while (module)
    {
        IM3Module next = module->next;
        r = i_visitor (module, i_info);
        if (r)
            break;

        module = next;
    }

    return r;
}


void *  _FreeModule  (IM3Module i_module, void * i_info)
{
    m3_FreeModule (i_module);
    return NULL;
}


void  Runtime_Release  (IM3Runtime i_runtime)
{
    ForEachModule (i_runtime, _FreeModule, NULL);                   d_m3Assert (i_runtime->numActiveCodePages == 0);

    Environment_ReleaseCodePages (i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages (i_runtime->environment, i_runtime->pagesFull);

    m3_Int_Free (i_runtime->originStack);

    void* memory_ptr = &i_runtime->memory;
    m3_FreeMemory (memory_ptr);
}


void  m3_FreeRuntime  (IM3Runtime i_runtime)
{
    if (i_runtime)
    {
        m3_PrintProfilerInfo ();

        Runtime_Release (i_runtime);
        m3_Int_Free (i_runtime);
    }
}

M3Result  EvaluateExpression  (IM3Module i_module, void * o_expressed, u8 i_type, bytes_t * io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#if defined(d_m3PreferStaticAlloc)
    static M3Runtime runtime;
#else
    M3Runtime runtime;
#endif
    M3_INIT (runtime);

    runtime.environment = i_module->runtime->environment;
    runtime.numStackSlots = i_module->runtime->numStackSlots;
    runtime.stack = i_module->runtime->stack;

    m3stack_t stack = (m3stack_t)runtime.stack;

    IM3Runtime savedRuntime = i_module->runtime;
    i_module->runtime = & runtime;

    IM3Compilation o = & runtime.compilation;
    o->runtime = & runtime;
    o->module =  i_module;
    o->wasm =    * io_bytes;
    o->wasmEnd = i_end;
    o->lastOpcodeStart = o->wasm;

    o->block.depth = -1;  // so that root compilation depth = 0

    //  OPTZ: this code page could be erased after use.  maybe have 'empty' list in addition to full and open?
    o->page = AcquireCodePage (& runtime);  // AcquireUnusedCodePage (...)

    if (o->page)
    {
        IM3FuncType ftype = runtime.environment->retFuncTypes[i_type];

        pc_t m3code = GetPagePC (o->page);
        result = CompileBlock (o, ftype, c_waOp_block);

        if (not result && o->maxStackSlots >= runtime.numStackSlots) {
            result = m3Err_trapStackOverflow;
        }

        if (not result)
        {
# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode (m3code, stack, NULL, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
            m3ret_t r = RunCode (m3code, stack, NULL, d_m3OpDefaultArgs);
# endif
            
            if (r == 0)
            {                                                                               m3log (runtime, "expression result: %s", SPrintValue (stack, i_type));
                if (SizeOfType (i_type) == sizeof (u32))
                {
                    * (u32 *) o_expressed = * ((u32 *) stack);
                }
                else
                {
                    * (u64 *) o_expressed = * ((u64 *) stack);
                }
            }
        }

        // TODO: EraseCodePage (...) see OPTZ above
        ReleaseCodePage (& runtime, o->page);
    }
    else result = m3Err_mallocFailedCodePage;

    runtime.originStack = NULL;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release (& runtime);
    i_module->runtime = savedRuntime;

    * io_bytes = o->wasm;

    return result;
}


/*M3Result  InitMemory  (IM3Runtime io_runtime, IM3Module i_module)
{
    if (i_module->memoryInfo.pageSize == 0) { //todo: use costants or external settings
        i_module->memoryInfo.pageSize = 65524; // 65536 - 12  
        ESP_LOGI("WASM3", "InitMemory - Fixed pageSize to standard 64KB");
    }

    #ifdef _DEBUG_MEMORY
     // Prima del calcolo
    ESP_LOGI("WASM3", "InitMemory - Module memory info: initial pages: %lu, max pages: %lu", 
            (unsigned long)i_module->memoryInfo.initPages,
            (unsigned long)i_module->memoryInfo.maxPages);

    ESP_LOGI("WASM3", "InitMemory - Page size: %lu bytes", 
            (unsigned long)i_module->memoryInfo.pageSize);
    #endif

    // Calcolo con verifica overflow
    size_t alloc_size = 0;
    if (i_module->memoryInfo.pageSize > 0) {  // previene divisione per zero
        alloc_size = (size_t)i_module->memoryInfo.initPages * i_module->memoryInfo.pageSize;
        
        // Verifica overflow
        if (alloc_size / i_module->memoryInfo.pageSize != i_module->memoryInfo.initPages) {
            ESP_LOGE("WASM3", "Memory size calculation overflow!");
            return "memory size overflow";
        }
    }

    M3Result result = m3Err_none;                                     //d_m3Assert (not io_runtime->memory.wasmPages);

    if (not i_module->memoryImported)
    {
        u32 maxPages = i_module->memoryInfo.maxPages;
        u32 pageSize = i_module->memoryInfo.pageSize;
        io_runtime->memory.maxPages = maxPages ? maxPages : 65536;
        io_runtime->memory.pageSize = pageSize ? pageSize : d_m3DefaultMemPageSize;

        ESP_LOGI("WASM3", "InitMemory - ResizeMemory"); 
        result = ResizeMemory (io_runtime, i_module->memoryInfo.initPages);
    }

    #ifdef _DEBUG_MEMORY
    ESP_LOGI("WASM3", "InitMemory - Calculated allocation size: %lu bytes", 
            (unsigned long)alloc_size);
    #endif


    return result;
}


M3Result  ResizeMemory  (IM3Runtime io_runtime, u32 i_numPages)
{
    M3Result result = m3Err_none;

    u32 numPagesToAlloc = i_numPages;

    M3Memory * memory = & io_runtime->memory;

#if 0 // Temporary fix for memory allocation
    if (memory->mallocated) {
        memory->numPages = i_numPages;
        memory->mallocated->end = memory->wasmPages + (memory->numPages * io_runtime->memory.pageSize);
        return result;
    }

    i_numPagesToAlloc = 256;
#endif

    if (numPagesToAlloc <= memory->maxPages)
    {
        size_t numPageBytes = numPagesToAlloc * io_runtime->memory.pageSize;

#if d_m3MaxLinearMemoryPages > 0
        _throwif("linear memory limitation exceeded", numPagesToAlloc > d_m3MaxLinearMemoryPages);
#endif

        // Limit the amount of memory that gets actually allocated
        if (io_runtime->memoryLimit) {
            numPageBytes = M3_MIN (numPageBytes, io_runtime->memoryLimit);
        }

        size_t numBytes = numPageBytes + sizeof (M3MemoryHeader);

        size_t numPreviousBytes = memory->numPages * io_runtime->memory.pageSize;
        if (numPreviousBytes)
            numPreviousBytes += sizeof (M3MemoryHeader);

        void* newMem = m3_Realloc ("Wasm Linear Memory", memory->mallocated, numBytes, numPreviousBytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

# if d_m3LogRuntime
        M3MemoryHeader * oldMallocated = memory->mallocated;
# endif

        memory->numPages = numPagesToAlloc;

        memory->mallocated->length =  numPageBytes;
        memory->mallocated->runtime = io_runtime;

        memory->mallocated->maxStack = (m3slot_t *) io_runtime->stack + io_runtime->numStackSlots;

        m3log (runtime, "resized old: %p; mem: %p; length: %zu; pages: %d", oldMallocated, memory->mallocated, memory->mallocated->length, memory->numPages);
    }
    else result = m3Err_wasmMemoryOverflow;

    _catch: return result;
}
*/

///
/// Ad hoc implementation
///

//#include <stdlib.h> // For malloc, realloc, free
//#include <string.h> // For memcpy

static const int DEBUG_TOP_MEMORY = 1;

M3Result InitMemory(IM3Runtime io_runtime, IM3Module i_module) {
    if(DEBUG_TOP_MEMORY) ESP_LOGI("WASM3", "InitMemory called");

    if (i_module->memoryInfo.pageSize == 0) {
        i_module->memoryInfo.pageSize = 65536;  // Standard WebAssembly page size
        ESP_LOGI("WASM3", "InitMemory - Fixed pageSize to standard 64KB");
    }

    M3Result result = m3Err_none;

    if (!i_module->memoryImported) {
        io_runtime->memory.maxPages = i_module->memoryInfo.maxPages ? 
                                    i_module->memoryInfo.maxPages : 65536;
        io_runtime->memory.pageSize = i_module->memoryInfo.pageSize;
        io_runtime->memory.segment_size = WASM_SEGMENT_SIZE;  // Usa la page size come segment size
        
        result = ResizeMemory(io_runtime, i_module->memoryInfo.initPages);
    }
    else {
        if(io_runtime->memory.segment_size == 0) {
            io_runtime->memory.segment_size = WASM_SEGMENT_SIZE;
        }
    }

    return result;
}

M3Result ResizeMemory(IM3Runtime io_runtime, u32 i_numPages) {
    if(DEBUG_TOP_MEMORY) ESP_LOGI("WASM3", "ResizeMemory called");

    M3Memory* memory = &io_runtime->memory;

    if (i_numPages <= memory->maxPages) {
        size_t numPageBytes = (size_t)i_numPages * memory->pageSize;

        if (numPageBytes / memory->pageSize != i_numPages) {
            ESP_LOGE("WASM3", "Memory size calculation overflow!");
            return m3Err_mallocFailed;
        }

        #if d_m3MaxLinearMemoryPages > 0
        if (i_numPages > d_m3MaxLinearMemoryPages) {
            ESP_LOGE("WASM3", "linear memory limitation exceeded");
            return m3Err_mallocFailed;
        }
        #endif

        if (io_runtime->memoryLimit && numPageBytes > io_runtime->memoryLimit) {
            numPageBytes = io_runtime->memoryLimit;
            i_numPages = numPageBytes / memory->pageSize;
        }

        // Calcola il numero di segmenti necessari 
        // (un segmento per pagina, dato che segment_size = pageSize)
        size_t num_segments = i_numPages;

        // Gestione dei segmenti
        if (!memory->segments) {
            // Allocazione iniziale dell'array di strutture MemorySegment
            memory->segments = current_allocator->malloc(
                num_segments * sizeof(MemorySegment)
            );
            if (!memory->segments){ 
                ESP_LOGE("WASM3", "Failed memory->segment allocation");
                return m3Err_mallocFailed;
            }
            
            // Inizializza tutte le strutture dei segmenti
            for (size_t i = 0; i < num_segments; i++) {
                memory->segments[i].data = NULL;
                memory->segments[i].is_allocated = false;
                //memory->segment_size = memory->pageSize; // FUCK ME
            }
        }
        else if (num_segments != memory->num_segments) {
            // Rialloca l'array di strutture MemorySegment
            MemorySegment* new_segments = current_allocator->realloc(
                memory->segments,
                num_segments * sizeof(MemorySegment)
            );
            if (!new_segments){
                ESP_LOGE("WASM3", "Failed new_segments allocation");
                return m3Err_mallocFailed;
            }
            
            memory->segments = new_segments;

            // Se stiamo crescendo, inizializza i nuovi segmenti
            if (num_segments > memory->num_segments) {
                for (size_t i = memory->num_segments; i < num_segments; i++) {
                    memory->segments[i].data = NULL;
                    memory->segments[i].is_allocated = false;
                    //memory->segment_size = memory->pageSize;
                }
            }
            else {
                // Se stiamo riducendo, libera la memoria dei segmenti in eccesso
                for (size_t i = num_segments; i < memory->num_segments; i++) {
                    if (memory->segments[i].is_allocated && memory->segments[i].data) {
                        current_allocator->free(memory->segments[i].data);
                    }
                }
            }
        }

        // Aggiorna i metadati della memoria
        memory->num_segments = num_segments;
        memory->numPages = i_numPages;
        memory->total_size = numPageBytes;
        
        ESP_LOGI("WASM3", "Memory resized to %lu pages (%zu bytes, %zu segments)", 
                 i_numPages, numPageBytes, num_segments);
    }
    else {
        return m3Err_wasmMemoryOverflow;
    }

    return m3Err_none;
}

void FreeMemory(IM3Runtime io_runtime) {
    if(DEBUG_TOP_MEMORY) ESP_LOGI("WASM3", "FreeMemory called");
    
    M3Memory* memory = &io_runtime->memory;

    if (memory->segments) {
        // Libera la memoria di ogni segmento allocato
        for (size_t i = 0; i < memory->num_segments; i++) {
            if (memory->segments[i].is_allocated && memory->segments[i].data) {
                m3_Free_Impl(memory->segments[i].data, false);
            }
        }
        // Libera l'array dei segmenti
        m3_Free_Impl(memory->segments, false);
        memory->segments = NULL;
        memory->num_segments = 0;
    }

    memory->numPages = 0;
    memory->maxPages = 0;
}

///
///
///

M3Result  InitGlobals  (IM3Module io_module)
{
    M3Result result = m3Err_none;

    if (io_module->numGlobals)
    {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            for (u32 i = 0; i < io_module->numGlobals; ++i)
            {
                M3Global * g = & io_module->globals [i];                        m3log (runtime, "initializing global: %d", i);

                if (g->initExpr)
                {
                    bytes_t start = g->initExpr;

                    result = EvaluateExpression (io_module, & g->i64Value, g->type, & start, g->initExpr + g->initExprSize);

                    if (not result)
                    {
                        // io_module->globalMemory [i] = initValue;
                    }
                    else break;
                }
                else
                {                                                               m3log (runtime, "importing global");

                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    return result;
}


M3Result InitDataSegments(M3Memory* io_memory, IM3Module io_module)
{
    M3Result result = m3Err_none;
    
    // Verifica che la struttura di memoria sia inizializzata
    _throwif("uninitialized memory structure", !io_memory || !io_memory->segments);

    for (u32 i = 0; i < io_module->numDataSegments; ++i)
    {
        M3DataSegment* segment = &io_module->dataSegments[i];

        i32 segmentOffset;
        bytes_t start = segment->initExpr;
_       (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i32, &start, 
                           segment->initExpr + segment->initExprSize));

        m3log(runtime, "loading data segment: %d; size: %d; offset: %d", 
              i, segment->size, segmentOffset);

        // Verifica limiti
        if (segmentOffset >= 0 && (size_t)(segmentOffset) + segment->size <= io_memory->total_size)
        {
            // Calcola i segmenti interessati
            size_t start_segment = segmentOffset / io_memory->segment_size;
            size_t end_segment = (segmentOffset + segment->size - 1) / io_memory->segment_size;
            
            // Alloca tutti i segmenti necessari se non sono già allocati
            for (size_t seg = start_segment; seg <= end_segment; seg++)
            {
                if (!io_memory->segments[seg].is_allocated) {
                    if (!allocate_segment(io_memory, seg)) {
                        _throw("failed to allocate memory segment");
                    }
                }
            }
            
            // Copia i dati attraverso i segmenti
            size_t remaining = segment->size;
            size_t src_offset = 0;
            size_t dest_offset = segmentOffset;
            
            while (remaining > 0)
            {
                size_t current_segment = dest_offset / io_memory->segment_size;
                size_t segment_offset = dest_offset % io_memory->segment_size;
                size_t bytes_to_copy = M3_MIN(
                    remaining,
                    io_memory->segment_size - segment_offset
                );
                
                u8* dest = ((u8*)io_memory->segments[current_segment].data) + segment_offset;
                memcpy(dest, segment->data + src_offset, bytes_to_copy);
                
                remaining -= bytes_to_copy;
                src_offset += bytes_to_copy;
                dest_offset += bytes_to_copy;
            }
        }
        else {
            _throw("data segment out of bounds");
        }
    }

    _catch: return result;
}


M3Result  InitElements  (IM3Module io_module)
{
    M3Result result = m3Err_none;

    bytes_t bytes = io_module->elementSection;
    cbytes_t end = io_module->elementSectionEnd;

    for (u32 i = 0; i < io_module->numElementSegments; ++i)
    {
        u32 index;
_       (ReadLEB_u32 (& index, & bytes, end));

        if (index == 0)
        {
            i32 offset;
_           (EvaluateExpression (io_module, & offset, c_m3Type_i32, & bytes, end));
            _throwif ("table underflow", offset < 0);

            u32 numElements;
_           (ReadLEB_u32 (& numElements, & bytes, end));

            size_t endElement = (size_t) numElements + offset;
            _throwif ("table overflow", endElement > d_m3MaxSaneTableSize);

            // is there any requirement that elements must be in increasing sequence?
            // make sure the table isn't shrunk.
            if (endElement > io_module->table0Size)
            {
                io_module->table0 = m3_Int_ReallocArray (IM3Function, io_module->table0, endElement, io_module->table0Size);
                io_module->table0Size = (u32) endElement;
            }
            _throwifnull(io_module->table0);

            for (u32 e = 0; e < numElements; ++e)
            {
                u32 functionIndex;
_               (ReadLEB_u32 (& functionIndex, & bytes, end));
                _throwif ("function index out of range", functionIndex >= io_module->numFunctions);
                IM3Function function = & io_module->functions [functionIndex];      d_m3Assert (function); //printf ("table: %s\n", m3_GetFunctionName(function));
                io_module->table0 [e + offset] = function;
            }
        }
        else _throw ("element table index must be zero for MVP");
    }

    _catch: return result;
}

M3Result  m3_CompileModule  (IM3Module io_module)
{
    M3Result result = m3Err_none;

    for (u32 i = 0; i < io_module->numFunctions; ++i)
    {
        IM3Function f = & io_module->functions [i];
        if (f->wasm and not f->compiled)
        {
_           (CompileFunction (f));
        }
    }

    _catch: return result;
}

M3Result  m3_RunStart  (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    return m3Err_none;
#endif

    M3Result result = m3Err_none;
    i32 startFunctionTmp = -1;

    if (io_module and io_module->startFunction >= 0)
    {
        IM3Function function = & io_module->functions [io_module->startFunction];

        if (not function->compiled)
        {
_           (CompileFunction (function));
        }

        IM3FuncType ftype = function->funcType;
        if (ftype->numArgs != 0 || ftype->numRets != 0)
            _throw (m3Err_argumentCountMismatch);

        IM3Module module = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;
        io_module->startFunction = -1;

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
        result = (M3Result) RunCode (function->compiled, (m3stack_t) runtime->stack, runtime->memory, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
        result = (M3Result) RunCode (function->compiled, (m3stack_t) runtime->stack, &runtime->memory, d_m3OpDefaultArgs);
# endif

        if (result)
        {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            goto _catch;
        }
    }

    _catch: return result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result  m3_LoadModule  (IM3Runtime io_runtime, IM3Module io_module)
{
    // Debug memory allocation
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    ESP_LOGI("WASM3", "Before LoadModule - Free: %d bytes, Largest block: %d bytes", 
         info.total_free_bytes, info.largest_free_block);

    // Start
    M3Result result = m3Err_none;

    if (M3_UNLIKELY(io_module->runtime)) {
        return m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;
    M3Memory * memory = & io_runtime->memory;    

    ESP_LOGI("WASM3", "Starting InitMemory");
_   (InitMemory (io_runtime, io_module));
    ESP_LOGI("WASM3", "InitMemory completed");

    ESP_LOGI("WASM3", "Starting InitGlobals");
_   (InitGlobals (io_module));
    ESP_LOGI("WASM3", "InitGlobals completed");

    ESP_LOGI("WASM3", "Starting InitDataSegments");
_   (InitDataSegments (memory, io_module));
    ESP_LOGI("WASM3", "InitDataSegments completed");

    ESP_LOGI("WASM3", "Starting InitElements");
_   (InitElements (io_module));
    ESP_LOGI("WASM3", "InitElements completed");

#ifdef DEBUG
    Module_GenerateNames(io_module);
#endif

    io_module->next = io_runtime->modules;
    io_runtime->modules = io_module;
    return result; // ok

_catch:
    // Log di debug per capire dove è fallito
    heap_caps_get_info(&info, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    ESP_LOGE("WASM3", "LoadModule failed - Free: %d bytes, Largest block: %d bytes", 
         info.total_free_bytes, info.largest_free_block);
    
    io_module->runtime = NULL;
    return result;
}

IM3Global  m3_FindGlobal  (IM3Module               io_module,
                           const char * const      i_globalName)
{
    // Search exports
    for (u32 i = 0; i < io_module->numGlobals; ++i)
    {
        IM3Global g = & io_module->globals [i];
        if (g->name and strcmp (g->name, i_globalName) == 0)
        {
            return g;
        }
    }

    // Search imports
    for (u32 i = 0; i < io_module->numGlobals; ++i)
    {
        IM3Global g = & io_module->globals [i];

        if (g->import.moduleUtf8 and g->import.fieldUtf8)
        {
            if (strcmp (g->import.fieldUtf8, i_globalName) == 0)
            {
                return g;
            }
        }
    }
    return NULL;
}

M3Result  m3_GetGlobal  (IM3Global                 i_global,
                         IM3TaggedValue            o_value)
{
    if (not i_global) return m3Err_globalLookupFailed;

    switch (i_global->type) {
    case c_m3Type_i32: o_value->value.i32 = i_global->i32Value; break;
    case c_m3Type_i64: o_value->value.i64 = i_global->i64Value; break;
# if d_m3HasFloat
    case c_m3Type_f32: o_value->value.f32 = i_global->f32Value; break;
    case c_m3Type_f64: o_value->value.f64 = i_global->f64Value; break;
# endif
    default: return m3Err_invalidTypeId;
    }

    o_value->type = (M3ValueType)(i_global->type);
    return m3Err_none;
}

M3Result  m3_SetGlobal  (IM3Global                 i_global,
                         const IM3TaggedValue      i_value)
{
    if (not i_global) return m3Err_globalLookupFailed;
    if (not i_global->isMutable) return m3Err_globalNotMutable;
    if (i_global->type != i_value->type) return m3Err_globalTypeMismatch;

    switch (i_value->type) {
    case c_m3Type_i32: i_global->i32Value = i_value->value.i32; break;
    case c_m3Type_i64: i_global->i64Value = i_value->value.i64; break;
# if d_m3HasFloat
    case c_m3Type_f32: i_global->f32Value = i_value->value.f32; break;
    case c_m3Type_f64: i_global->f64Value = i_value->value.f64; break;
# endif
    default: return m3Err_invalidTypeId;
    }

    return m3Err_none;
}

M3ValueType  m3_GetGlobalType  (IM3Global          i_global)
{
    return (i_global) ? (M3ValueType)(i_global->type) : c_m3Type_none;
}


static const bool WASM_DEBUG_VERBOSE_v_FindFunction = false;
void *  v_FindFunction  (IM3Module i_module, const char * const i_name)
{
    // Prefer exported functions
    for (u32 i = 0; i < i_module->numFunctions; ++i)
    {
        IM3Function f = & i_module->functions [i];

        if(WASM_DEBUG_VERBOSE_v_FindFunction){
            if(f->export_name){
                ESP_LOGI("WASM3", "v_FindFunction: cycling function %s in module %s", f->export_name, i_module->name);
            }
        }

        if (f->export_name and strcmp (f->export_name, i_name) == 0)
            return f;
    }

    // Search internal functions
    for (u32 i = 0; i < i_module->numFunctions; ++i)
    {
        IM3Function f = & i_module->functions [i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        if (isImported)
            continue;

        for (int j = 0; j < f->numNames; j++)
        {
            if(WASM_DEBUG_VERBOSE_v_FindFunction){
                if(f->names [j]){
                    ESP_LOGI("WASM3", "v_FindFunction: cycling internal function %s in module %s", f->names [j], i_module->name);
                }
            }

            if (f->names [j] and strcmp (f->names [j], i_name) == 0)
                return f;
        }
    }

    return NULL;
}


M3Result  m3_FindFunction  (IM3Function * o_function, IM3Runtime i_runtime, const char * const i_functionName)
{
    M3Result result = m3Err_none;                               d_m3Assert (o_function and i_runtime and i_functionName);

    IM3Function function = NULL;

    if (not i_runtime->modules) {
        _throw ("no modules loaded");
    }

    function = (IM3Function) ForEachModule (i_runtime, (ModuleVisitor) v_FindFunction, (void *) i_functionName);

    if (function)
    {
        if (not function->compiled)
        {
_           (CompileFunction (function))
        }
    }
    else _throw (ErrorModule (m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName));

    _catch:
    if (result)
        function = NULL;

    * o_function = function;

    return result;
}


M3Result  m3_GetTableFunction  (IM3Function * o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    if (i_index >= i_module->table0Size)
    {
        _throw ("function index out of range");
    }

    IM3Function function = i_module->table0[i_index];

    if (function)
    {
        if (not function->compiled)
        {
_           (CompileFunction (function))
        }
    }

    * o_function = function;
}   _catch:
    return result;
}


static
M3Result checkStartFunction(IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    if (i_module->startFunction >= 0)
    {
        result = m3_RunStart (i_module);
    }

    return result;
}

uint32_t  m3_GetArgCount  (IM3Function i_function)
{
    if (i_function) {
        IM3FuncType ft = i_function->funcType;
        if (ft) {
            return ft->numArgs;
        }
    }
    return 0;
}

uint32_t  m3_GetRetCount  (IM3Function i_function)
{
    if (i_function) {
        IM3FuncType ft = i_function->funcType;
        if (ft) {
            return ft->numRets;
        }
    }
    return 0;
}


M3ValueType  m3_GetArgType  (IM3Function i_function, uint32_t index)
{
    if (i_function) {
        IM3FuncType ft = i_function->funcType;
        if (ft and index < ft->numArgs) {
            return (M3ValueType)d_FuncArgType(ft, index);
        }
    }
    return c_m3Type_none;
}

M3ValueType  m3_GetRetType  (IM3Function i_function, uint32_t index)
{
    if (i_function) {
        IM3FuncType ft = i_function->funcType;
        if (ft and index < ft->numRets) {
            return (M3ValueType) d_FuncRetType (ft, index);
        }
    }
    return c_m3Type_none;
}


u8 *  GetStackPointerForArgs  (IM3Function i_function)
{
    u64 * stack = (u64 *) i_function->module->runtime->stack;
    IM3FuncType ftype = i_function->funcType;

    stack += ftype->numRets;

    return (u8 *) stack;
}


M3Result  m3_CallV  (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    return r;
}

static
void  ReportNativeStackUsage  ()
{
#   if d_m3LogNativeStack
        int stackUsed =  m3StackGetMax();
        fprintf (stderr, "Native stack used: %d\n", stackUsed);
#   endif
}


M3Result m3_CallVL(IM3Function i_function, va_list i_args)
{
    IM3Runtime runtime = i_function->module->runtime;
    IM3FuncType ftype = i_function->funcType;
    M3Result result = m3Err_none;
    u8* s = NULL;

    if (!i_function->compiled) {
        return m3Err_missingCompiledCode;
    }

# if d_m3RecordBacktraces
    ClearBacktrace(runtime);
# endif

    m3StackCheckInit();

_   (checkStartFunction(i_function->module))

    s = GetStackPointerForArgs(i_function);

    for (u32 i = 0; i < ftype->numArgs; ++i)
    {
        switch (d_FuncArgType(ftype, i)) {
        case c_m3Type_i32:  *(i32*)(s) = va_arg(i_args, i32);  s += 8; break;
        case c_m3Type_i64:  *(i64*)(s) = va_arg(i_args, i64);  s += 8; break;
# if d_m3HasFloat
        case c_m3Type_f32:  *(f32*)(s) = va_arg(i_args, f64);  s += 8; break;
        case c_m3Type_f64:  *(f64*)(s) = va_arg(i_args, f64);  s += 8; break;
# endif
        default: return "unknown argument type";
        }
    }

// Here's born _mem
# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    result = (M3Result) RunCode(i_function->compiled, (m3stack_t)(runtime->stack), &runtime->memory, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
    result = (M3Result) RunCode(i_function->compiled, (m3stack_t)(runtime->stack), &runtime->memory, d_m3OpDefaultArgs);
# endif
    ReportNativeStackUsage();

    runtime->lastCalled = result ? NULL : i_function;

    _catch: return result;
}

M3Result m3_Call(IM3Function i_function, uint32_t i_argc, const void* i_argptrs[])
{
    IM3Runtime runtime = i_function->module->runtime;
    IM3FuncType ftype = i_function->funcType;
    M3Result result = m3Err_none;
    u8* s = NULL;

    if (i_argc != ftype->numArgs) {
        return m3Err_argumentCountMismatch;
    }
    if (!i_function->compiled) {
        return m3Err_missingCompiledCode;
    }

# if d_m3RecordBacktraces
    ClearBacktrace(runtime);
# endif

    m3StackCheckInit();

_   (checkStartFunction(i_function->module))

    s = GetStackPointerForArgs(i_function);

    for (u32 i = 0; i < ftype->numArgs; ++i)
    {
        switch (d_FuncArgType(ftype, i)) {
        case c_m3Type_i32:  *(i32*)(s) = *(i32*)i_argptrs[i];  s += 8; break;
        case c_m3Type_i64:  *(i64*)(s) = *(i64*)i_argptrs[i];  s += 8; break;
# if d_m3HasFloat
        case c_m3Type_f32:  *(f32*)(s) = *(f32*)i_argptrs[i];  s += 8; break;
        case c_m3Type_f64:  *(f64*)(s) = *(f64*)i_argptrs[i];  s += 8; break;
# endif
        default: return "unknown argument type";
        }
    }

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    result = (M3Result) RunCode(i_function->compiled, (m3stack_t)(runtime->stack), &runtime->memory, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
    result = (M3Result) RunCode(i_function->compiled, (m3stack_t)(runtime->stack), &runtime->memory, d_m3OpDefaultArgs);
# endif

    ReportNativeStackUsage();

    runtime->lastCalled = result ? NULL : i_function;

    _catch: return result;
}

M3Result m3_CallArgv(IM3Function i_function, uint32_t i_argc, const char* i_argv[])
{
    IM3FuncType ftype = i_function->funcType;
    IM3Runtime runtime = i_function->module->runtime;
    M3Result result = m3Err_none;
    u8* s = NULL;

    if (i_argc != ftype->numArgs) {
        return m3Err_argumentCountMismatch;
    }
    if (!i_function->compiled) {
        return m3Err_missingCompiledCode;
    }

# if d_m3RecordBacktraces
    ClearBacktrace(runtime);
# endif

    m3StackCheckInit();

_   (checkStartFunction(i_function->module))

    s = GetStackPointerForArgs(i_function);

    for (u32 i = 0; i < ftype->numArgs; ++i)
    {
        switch (d_FuncArgType(ftype, i)) {
        case c_m3Type_i32:  *(i32*)(s) = strtoul(i_argv[i], NULL, 10);  s += 8; break;
        case c_m3Type_i64:  *(i64*)(s) = strtoull(i_argv[i], NULL, 10); s += 8; break;
# if d_m3HasFloat
        case c_m3Type_f32:  *(f32*)(s) = strtod(i_argv[i], NULL);       s += 8; break;
        case c_m3Type_f64:  *(f64*)(s) = strtod(i_argv[i], NULL);       s += 8; break;
# endif
        default: return "unknown argument type";
        }
    }

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    result = (M3Result) RunCode(i_function->compiled, (m3stack_t)(runtime->stack), &runtime->memory, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
    result = (M3Result) RunCode(i_function->compiled, (m3stack_t)(runtime->stack), &runtime->memory, d_m3OpDefaultArgs);
# endif
    
    ReportNativeStackUsage();

    runtime->lastCalled = result ? NULL : i_function;

    _catch: return result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result  m3_GetResults  (IM3Function i_function, uint32_t i_retc, const void * o_retptrs[])
{
    IM3FuncType ftype = i_function->funcType;
    IM3Runtime runtime = i_function->module->runtime;

    if (i_retc != ftype->numRets) {
        return m3Err_argumentCountMismatch;
    }
    if (i_function != runtime->lastCalled) {
        return "function not called";
    }

    u8* s = (u8*) runtime->stack;

    for (u32 i = 0; i < ftype->numRets; ++i)
    {
        switch (d_FuncRetType(ftype, i)) {
        case c_m3Type_i32:  *(i32*)o_retptrs[i] = *(i32*)(s); s += 8; break;
        case c_m3Type_i64:  *(i64*)o_retptrs[i] = *(i64*)(s); s += 8; break;
# if d_m3HasFloat
        case c_m3Type_f32:  *(f32*)o_retptrs[i] = *(f32*)(s); s += 8; break;
        case c_m3Type_f64:  *(f64*)o_retptrs[i] = *(f64*)(s); s += 8; break;
# endif
        default: return "unknown return type";
        }
    }
    return m3Err_none;
}

M3Result  m3_GetResultsV  (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    return r;
}

M3Result  m3_GetResultsVL  (IM3Function i_function, va_list o_rets)
{
    IM3Runtime runtime = i_function->module->runtime;
    IM3FuncType ftype = i_function->funcType;

    if (i_function != runtime->lastCalled) {
        return "function not called";
    }

    u8* s = (u8*) runtime->stack;
    for (u32 i = 0; i < ftype->numRets; ++i)
    {
        switch (d_FuncRetType(ftype, i)) {
        case c_m3Type_i32:  *va_arg(o_rets, i32*) = *(i32*)(s);  s += 8; break;
        case c_m3Type_i64:  *va_arg(o_rets, i64*) = *(i64*)(s);  s += 8; break;
# if d_m3HasFloat
        case c_m3Type_f32:  *va_arg(o_rets, f32*) = *(f32*)(s);  s += 8; break;
        case c_m3Type_f64:  *va_arg(o_rets, f64*) = *(f64*)(s);  s += 8; break;
# endif
        default: return "unknown argument type";
        }
    }
    return m3Err_none;
}

void  ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    if (i_codePage)
    {
        IM3CodePage * list;

        bool pageFull = (NumFreeLines (i_codePage) < d_m3CodePageFreeLinesThreshold);
        if (pageFull)
            list = & i_runtime->pagesFull;
        else
            list = & i_runtime->pagesOpen;

        PushCodePage (list, i_codePage);                        m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage  AcquireCodePageWithCapacity  (IM3Runtime i_runtime, u32 i_minLineCount)
{
    IM3CodePage page = RemoveCodePageOfCapacity (& i_runtime->pagesOpen, i_minLineCount);

    if (not page)
    {
        page = Environment_AcquireCodePage (i_runtime->environment, i_minLineCount);

        if (not page)
            page = NewCodePage (i_runtime, i_minLineCount);

        if (page)
            i_runtime->numCodePages++;
    }

    if (page)
    {                                                            m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->numActiveCodePages++;
    }

    return page;
}


IM3CodePage  AcquireCodePage  (IM3Runtime i_runtime)
{
    return AcquireCodePageWithCapacity (i_runtime, d_m3CodePageFreeLinesThreshold);
}


void  ReleaseCodePage  (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    if (i_codePage)
    {
        ReleaseCodePageNoTrack (i_runtime, i_codePage);
        i_runtime->numActiveCodePages--;

#       if defined (DEBUG)
            u32 numOpen = CountCodePages (i_runtime->pagesOpen);
            u32 numFull = CountCodePages (i_runtime->pagesFull);

            m3log (runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, numOpen, numFull, i_runtime->numActiveCodePages, i_runtime->numCodePages);

            d_m3Assert (numOpen + numFull + i_runtime->numActiveCodePages == i_runtime->numCodePages);

#           if d_m3LogCodePages
                dump_code_page (i_codePage, /* startPC: */ NULL);
#           endif
#       endif
    }
}


#if d_m3VerboseErrorMessages
M3Result  m3Error  (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                    const char * const i_file, u32 i_lineNum, const char * const i_errorMessage, ...)
{
    if (i_runtime)
    {
        i_runtime->error = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module,
                                          .function = i_function, .file = i_file, .line = i_lineNum };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start (args, i_errorMessage);
        vsnprintf (i_runtime->error_message, sizeof(i_runtime->error_message), i_errorMessage, args);
        va_end (args);
    }

    return i_result;
}
#endif


void  m3_GetErrorInfo  (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    if (i_runtime)
    {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo (i_runtime);
    }
}


void m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    if (i_runtime)
    {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory(IM3Runtime i_runtime, uint32_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = NULL;                                                    
    d_m3Assert(i_memoryIndex == 0);

    if (i_runtime && i_runtime->memory.segments)
    {
        uint32_t size = (uint32_t)i_runtime->memory.total_size;

        if (o_memorySizeInBytes)
            *o_memorySizeInBytes = size;

        if (size)
        {
            // Alloca un blocco contiguo di memoria
            memory = (uint8_t*)current_allocator->malloc(size);
            
            if (memory)
            {
                // Alloca tutti i segmenti necessari e copia i dati
                for (size_t i = 0; i < i_runtime->memory.num_segments; i++)
                {
                    // Alloca il segmento se non è già allocato
                    if (!i_runtime->memory.segments[i].is_allocated)
                    {
                        //size_t seg_size = (i == i_runtime->memory.num_segments - 1) ? (size - (i * i_runtime->memory.segment_size)) :  i_runtime->memory.segment_size;
                        size_t seg_size = i_runtime->memory.segment_size;

                        i_runtime->memory.segments[i].data = current_allocator->malloc(seg_size);
                        if (!i_runtime->memory.segments[i].data)
                        {
                            // Fallimento: libera tutto e ritorna NULL
                            for (size_t j = 0; j < i; j++) {
                                current_allocator->free(i_runtime->memory.segments[j].data);
                            }
                            current_allocator->free(memory);
                            return NULL;
                        }
                        i_runtime->memory.segments[i].is_allocated = true;
                        //i_runtime->memory.segments[i].size = seg_size;
                        memset(i_runtime->memory.segments[i].data, 0, seg_size);
                    }
                    
                    // Copia i dati dal segmento al blocco contiguo
                    size_t offset = i * i_runtime->memory.segment_size;
                    memcpy(memory + offset, 
                           i_runtime->memory.segments[i].data,
                           i_runtime->memory.segment_size);
                }
            }
        }
    }

    return memory;
}


uint32_t  m3_GetMemorySize  (IM3Runtime i_runtime)
{
    return i_runtime->memory.total_size;
}


M3BacktraceInfo *  m3_GetBacktrace  (IM3Runtime i_runtime)
{
# if d_m3RecordBacktraces
    return & i_runtime->backtrace;
# else
    return NULL;
# endif
}

