//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include <stdarg.h>
#include <limits.h>

#include "m3_env.h"
#include "m3_segmented_memory.h"
#include "wasm3.h"
#include "wasm3_defs.h"


#if PASSTHROUGH_HELLOESP
#include "esp_task_wdt.h"
#endif

DEBUG_TYPE WASM_DEBUG_NEW_ENV = WASM_DEBUG_ALL || (WASM_DEBUG && false);

IM3Environment  m3_NewEnvironment  ()
{
    if(WASM_DEBUG_NEW_ENV) ESP_LOGI("WASM3", "m3_NewEnvironment called");
    //init_globalMemory();
    if(WASM_DEBUG_NEW_ENV) ESP_LOGI("WASM3", "m3_NewEnvironment: init_globalMemory called");

    IM3Environment env = m3_Def_AllocStruct (M3Environment);
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
        m3_Def_Free (ftype);
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
        m3_Def_Free (i_environment);
    }
}


void m3_SetCustomSectionHandler  (IM3Environment i_environment, M3SectionHandler i_handler)
{
    if (i_environment) i_environment->customSectionHandler = i_handler;
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
DEBUG_TYPE WASM_DEBUG_ADDFUNC = WASM_DEBUG_ALL || (WASM_DEBUG && false);
void  Environment_AddFuncType  (IM3Environment i_environment, IM3FuncType * io_funcType)
{
    if(WASM_DEBUG_ADDFUNC) ESP_LOGI("WASM3", "Called Environment_AddFuncType");

    IM3FuncType addType = * io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    while (newType)
    {
        if (AreFuncTypesEqual (newType, addType))
        {
            m3_Def_Free (addType);
            break;
        }
        else {
            if(!ultra_safe_ptr_valid(addType)){
                ESP_LOGE("WASM3", "Invalid addType pointer in Environment_AddFuncType");
                return;
            }

            if(!ultra_safe_ptr_valid(newType)){
                ESP_LOGE("WASM3", "Invalid newType pointer in Environment_AddFuncType");
                m3_Def_Free (addType);
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

#define RemoveCodePage_Free 1
#if RemoveCodePage_Free

bool IsCodePageSafeToFree(IM3CodePage page) {
    // Verifica che la pagina non sia in uso
    if (page->info.usageCount > 0)
        return false;
        
    return true;
}

DEBUG_TYPE WASM_DEBUG_RemoveCodePageOfCapacity = WASM_DEBUG_ALL || (WASM_DEBUG && false);
IM3CodePage RemoveCodePageOfCapacity(M3CodePage ** io_list, u32 i_minimumLineCount)
{
    if(WASM_DEBUG_RemoveCodePageOfCapacity) ESP_LOGI("WASM3", "RemoveCodePageOfCapacity called (io_list=%p, i_minimumLineCount=%d)", io_list, i_minimumLineCount);

    IM3CodePage prev = NULL;
    IM3CodePage page = * io_list;

    while (page)
    {
        if(WASM_DEBUG_RemoveCodePageOfCapacity) ESP_LOGI("WASM3", "RemoveCodePageOfCapacity: freeing page %p", page);
        if (NumFreeLines(page) >= i_minimumLineCount)
        {
             if(WASM_DEBUG_RemoveCodePageOfCapacity) ESP_LOGI("WASM3", "RemoveCodePageOfCapacity: page->info.usageCount = %d", page->info.usageCount);
            d_m3Assert(page->info.usageCount == 0);
            
            // Verifica se è sicuro deallocare
            if (M3CodePage_RemoveCodePageOfCapacity_FreePage && IsCodePageSafeToFree(page))
            {
                if(WASM_DEBUG_RemoveCodePageOfCapacity) ESP_LOGI("WASM3", "RemoveCodePageOfCapacity: Freeing CodePage safe");

                IM3CodePage next = page->info.next;
                
                // Rimuovi dalla lista
                if (prev)
                    prev->info.next = next;
                else
                    *io_list = next;                                    
                
                // Dealloca la pagina
                m3_Def_Free(page);
                return NULL; // Indichiamo che la pagina è stata deallocata
            }
            else {
                if(M3CodePage_RemoveCodePageOfCapacity_FreePage){
                    if(WASM_DEBUG_RemoveCodePageOfCapacity) ESP_LOGW("WASM3", "RemoveCodePageOfCapacity: CodePage not safe to free");
                }

                // Se non è sicuro deallocare, comportamento originale
                IM3CodePage next = page->info.next;
                if (prev)
                    prev->info.next = next;
                else
                    *io_list = next;
                    
                break;
            }                        
        }

        prev = page;
        page = page->info.next;
    }

    return page;
}

#else
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
#endif

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

DEBUG_TYPE WASM_DEBUG_STACK = WASM_DEBUG_ALL || (WASM_DEBUG && false);
IM3Memory m3_NewStack(){
    if(WASM_DEBUG_STACK) ESP_LOGI("WASM3", "m3_NewStack called");

    IM3Memory memory = m3_NewMemory();
    
    if (memory == NULL){
        ESP_LOGE("WASM3", "Failed to allocate memory for IM3Memory");
        return NULL;
    }

    return memory;
}

DEBUG_TYPE WASM_DEBUG_NEW_RUNTIME = WASM_DEBUG_ALL || (WASM_DEBUG && false);
IM3Runtime  m3_NewRuntime  (IM3Environment i_environment, u32 i_stackSizeInBytes, void * i_userdata)
{
    if(WASM_DEBUG_NEW_RUNTIME) ESP_LOGI("WASM3", "m3_NewRuntime called");

    IM3Runtime runtime = m3_Def_AllocStruct (M3Runtime);
    if(WASM_DEBUG_NEW_RUNTIME) ESP_LOGI("WASM3", "m3_NewRuntime: m3_Def_AllocStruct done (%d bytes)", sizeof(M3Runtime));

    if (runtime)
    {        
        runtime->memory.runtime = runtime;    

        if(WASM_DEBUG_NEW_RUNTIME){
            ESP_LOGI("WASM3", "m3_NewRuntime: runtime ptr: %p", runtime);
            ESP_LOGI("WASM3", "m3_NewRuntime: runtime->memory ptr: %p", runtime->memory);
            ESP_LOGI("WASM3", "m3_NewRuntime: &runtime->memory ptr: %p", &runtime->memory);
        }

        IM3Memory memory = m3_InitMemory(&runtime->memory);
        memory->runtime = runtime;

        m3_ResetErrorInfo(runtime);
        if(WASM_DEBUG_NEW_RUNTIME) ESP_LOGI("WASM3", "m3_NewRuntime: m3_ResetErrorInfo done");

        runtime->environment = i_environment;
        runtime->userdata = i_userdata;

        /*runtime->originStack = 0;
        runtime->stack = runtime->originStack;
        runtime->maxStackSize = i_stackSizeInBytes; 
        runtime->numStackSlots = i_stackSizeInBytes / sizeof (m3slot_t);*/

        /// Preparing the stack is no more necessary      

        size_t stackSize = i_stackSizeInBytes + 4 * sizeof (m3slot_t);
        if(WASM_DEBUG_NEW_RUNTIME) {
            ESP_LOGI("WASM3", "m3_NewRuntime: allocating originStack (%d)", stackSize);
            ESP_LOGI("WASM3", "flush");
        }

        #if M3Runtime_Stack_Segmented
        runtime->originStack = m3_Malloc (memory, i_stackSizeInBytes + 4 * sizeof (m3slot_t)); // TODO: more precise stack checks
        #else
        runtime->originStack = m3_Def_Malloc (stackSize); // default malloc
        #endif 
        //runtime->originStack = m3_NewStack(); // (not implemented) ad hoc M3Memory for stack        
        

        if (runtime->originStack)
        {
            // Use in case of ad hoc M3Memory for stack
            //runtime->originStack->runtime = runtime;
            //runtime->originStack->max_size = i_stackSizeInBytes;

            runtime->stack = runtime->originStack;
            runtime->maxStackSize = i_stackSizeInBytes; 
            runtime->numStackSlots = i_stackSizeInBytes / sizeof (m3slot_t);         
            m3log (runtime, "new stack: %p", runtime->originStack);
        }
        else { 
            ESP_LOGE("WASM3", "m3_NewRuntime: runtime->originStack is NULL");
            m3_Def_Free (runtime);
            return NULL;
        }
    }
    else {
        ESP_LOGE("WASM3", "m3_NewRuntime: runtime is NULL");
    }    

    if(runtime == NULL) {
        if(WASM_DEBUG_NEW_RUNTIME) ESP_LOGI("WASM3", "m3_NewRuntime: successfully created");
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

    M3Memory* memory = &i_runtime->memory;
   
    if(i_runtime->originStack != NULL){
        #if M3Runtime_Stack_Segmented
        m3_free(memory, i_runtime->originStack); 
        #else        
        m3_Def_Free(i_runtime->originStack);
        #endif
    }
    
    FreeMemory (memory);
}

void  m3_FreeRuntime  (IM3Runtime i_runtime)
{
    if (i_runtime)
    {
        m3_PrintProfilerInfo ();

        Runtime_Release (i_runtime);        
    }
}

DEBUG_TYPE WASM_DEBUG_EvaluateExpression = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  EvaluateExpression  (IM3Module i_module, void * o_expressed, u8 i_type, bytes_t * io_bytes, cbytes_t i_end)
{
    CALL_WATCHDOG
    if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression called");

    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context

    #if defined(d_m3PreferStaticAlloc)
        static M3Runtime runtime = { 0 };
    #else
        M3Runtime runtime = { 0 };
    #endif    

    //M3_INIT (runtime);    

    if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression: M3Runtime size: %d", sizeof (M3Runtime));

    IM3Runtime savedRuntime = i_module->runtime;

    runtime.environment = savedRuntime->environment;
    runtime.numStackSlots = savedRuntime->numStackSlots; 
    runtime.stack = savedRuntime->stack;
    runtime.memory = savedRuntime->memory;

    m3stack_t stack = runtime.stack;

    ESP_LOGI("WASM3", "Stack pointer at: %p", stack);

    i_module->runtime = & runtime;

    IM3Compilation o = & runtime.compilation;
    o->runtime = i_module->runtime;
    o->module =  i_module;
    o->wasm =    * io_bytes;
    o->wasmEnd = i_end;
    o->lastOpcodeStart = o->wasm;

    o->block.depth = -1;  // so that root compilation depth = 0

    //  OPTZ: this code page could be erased after use.  maybe have 'empty' list in addition to full and open?
    if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression: AcquireCodePage");
    o->page = AcquireCodePage (& runtime);  // AcquireUnusedCodePage (...)

    if (o->page)
    {
        IM3FuncType ftype = runtime.environment->retFuncTypes[i_type];

        if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression: GetPagePC");

        pc_t m3code = GetPagePC (o->page);

        if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression: CompileBlock");
        result = CompileBlock (o, ftype, c_waOp_block);

        if (not result && o->maxStackSlots >= runtime.numStackSlots) {
            result = error_details(m3Err_trapStackOverflow, "in EvaluateExpression");
        }

        if (not result)
        {
            IM3Memory memory = &o->runtime->memory; //&runtime.memory; // &o->runtime->memory
            if(WASM_DEBUG_EvaluateExpression) {
                ESP_LOGI("WASM3", "EvaluateExpression: RunCode (m3code: %p, *m3code: %p, stack: %p, memory: %p)", m3code, *m3code , stack, memory);
                waitForIt();
            }

            # if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode (m3code, stack, memory , d_m3OpDefaultArgs, d_m3BaseCstr); // NULL or &o->runtime ?
            # else
            m3ret_t r = RunCode (m3code, stack, memory , d_m3OpDefaultArgs); 
            # endif
            
            if(WASM_DEBUG_EvaluateExpression) {
                ESP_LOGI("WASM3", "EvaluateExpression: RunCode r: %p", r);
            }

            if (r == 0)
            {                                                                               
                m3log (runtime, "expression result: %s", SPrintValue (stack, i_type));

                if (SizeOfType (i_type) == sizeof (u32))
                {
                    if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression: going to: * (u32 *) o_expressed = * ((u32 *) stack);");

                    #if M3Runtime_Stack_Segmented
                    //* (u32 *) o_expressed = * ((u32 *) m3_ResolvePointer(&i_module->runtime->memory, stack));
                    * (u32 *) m3_ResolvePointer(&i_module->runtime->memory, CAST_PTR o_expressed) = * ((u32 *) m3_ResolvePointer(&i_module->runtime->memory, stack));
                    #else 
                     * (u32 *) o_expressed = * ((u32 *) stack);
                    #endif
                }
                else
                {                    
                    if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression: going to: * (u64 *) o_expressed = * ((u64 *) stack);");

                    #if M3Runtime_Stack_Segmented
                    //* (u64 *) o_expressed = * ((u64 *) m3_ResolvePointer(&i_module->runtime->memory, stack));
                    * (u64 *) m3_ResolvePointer(&i_module->runtime->memory, o_expressed) = * ((u64 *) m3_ResolvePointer(&i_module->runtime->memory, stack));
                    #else 
                    * (u64 *) o_expressed = * ((u64 *) stack);
                    #endif
                }

                 if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression: cycle completed");
            }
        }

        // TODO: EraseCodePage (...) see OPTZ above
        if(WASM_DEBUG_EvaluateExpression) ESP_LOGI("WASM3", "EvaluateExpression: ReleaseCodePage");
        //ReleaseCodePage (& runtime, o->page);
    }
    else result = m3Err_mallocFailedCodePage;

    //runtime.originStack = NULL;        // prevent free(stack) in ReleaseRuntime
    //Runtime_Release (& runtime);
    i_module->runtime = savedRuntime;
    * io_bytes = o->wasm;

    if(WASM_DEBUG_EvaluateExpression){
        if(result != NULL){
            ESP_LOGI("WASM3", "EvaluateExpression: return %s", result);
        }
        else {
            ESP_LOGI("WASM3", "EvaluateExpression: return ok");
        }
    }

    return result;
}

///
/// M3MemoryHeader
///

DEBUG_TYPE WASM_DEBUG_ResizeMemory = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result ResizeMemory(IM3Runtime io_runtime, u32 i_numPages) {
    if(WASM_DEBUG_ResizeMemory) ESP_LOGI("WASM3", "ResizeMemory: i_numPages = %d", i_numPages);

    M3Result result = m3Err_none;
    
    if (!io_runtime) return m3Err_nullRuntime;
    
    M3Memory* memory = &io_runtime->memory;
    if (!IsValidMemory(memory)) return m3Err_malformedData;

    // Verifica limiti di memoria
    if (memory->maxPages > 0 && i_numPages > memory->maxPages) {
        return m3Err_wasmMemoryOverflow;
    }

    // Calcola la nuova dimensione totale richiesta
    size_t new_total_size = i_numPages * memory->pageSize;

    return GrowMemory(memory, new_total_size);

#if d_m3MaxLinearMemoryPages > 0
    _throwif("linear memory limitation exceeded", i_numPages > d_m3MaxLinearMemoryPages);
#endif
    
    new_total_size += memory->total_size; // add to total size

    // Applica il limite di memoria se impostato
    if (io_runtime->memoryLimit) {
        new_total_size = M3_MIN(new_total_size, io_runtime->memoryLimit);
    }

    // Calcola quanti nuovi segmenti sono necessari
    size_t segments_needed = (new_total_size + memory->segment_size - 1) / memory->segment_size;
    
    if (segments_needed > memory->num_segments) {
        // Dobbiamo aggiungere nuovi segmenti
        size_t additional_segments = segments_needed - memory->num_segments;
        
        result = AddSegments(memory, additional_segments);
        if (result) {
            return result;
        }
    }
    
    // Aggiorna la dimensione totale della memoria
    memory->total_size = new_total_size;
    
    // Inizializza i segmenti se necessario
    for (size_t i = 0; i < segments_needed; i++) {
        MemorySegment* seg = memory->segments[i];
        if (!seg || !seg->data) {
            if (InitSegment(memory, seg, true) == NULL) {
                continue;
            }
        }
    }

    _catch: return result;
}

////////////////////////////////////////////////////////////////////////

// Memory initialization M3Runtime - M3Module
DEBUG_TYPE WASM_DEBUG_INIT_MEMORY = WASM_DEBUG_ALL || (WASM_DEBUG && false);
const bool WASM_INIT_MEMORY_PREALLOC_SEGMENTS = false;
M3Result InitMemory(IM3Runtime io_runtime, IM3Module i_module) // todo: add to .h
{
    if(WASM_DEBUG_INIT_MEMORY) ESP_LOGI("WASM3", "InitMemory called");
    M3Result result = m3Err_none;

    if (not i_module->memoryImported)
    {
        if(io_runtime->memory.segment_size == 0){
            ESP_LOGE("WASM3", "InitMemory: o_runtime->memory.segment_size == 0");
        }

        m3_InitMemory(&io_runtime->memory);
        
        // Calcola dimensioni totali
        io_runtime->memory.max_size = 0;
        io_runtime->memory.segment_size = WASM_SEGMENT_SIZE;
        
        // Calcola numero iniziale di segmenti necessari
        if(WASM_INIT_MEMORY_PREALLOC_SEGMENTS){
            u32 pageSize = i_module->memoryInfo.pageSize;
            pageSize = pageSize ? pageSize : 65536;
            size_t initial_size = (size_t)i_module->memoryInfo.initPages * pageSize;
            size_t num_segments = (initial_size + io_runtime->memory.segment_size - 1) / io_runtime->memory.segment_size;

            if(WASM_DEBUG_INIT_MEMORY) ESP_LOGI("WASM3", "InitMemory: Malloc MemorySegment (num segments: %d)", num_segments);
            result = AddSegments(&io_runtime->memory, num_segments);
        }
    }

    return result;
}


///
///
///

DEBUG_TYPE WASM_DEBUG_InitGlobals = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  InitGlobals  (IM3Module io_module)
{
    M3Result result = m3Err_none;

    if(WASM_DEBUG_InitGlobals) ESP_LOGI("WASM3", "InitGlobals: io_module->numGlobals = %d", io_module->numGlobals);
    if (io_module->numGlobals)
    {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            for (u32 i = 0; i < io_module->numGlobals; ++i)
            {
                M3Global * g = & io_module->globals [i];                        
                m3log (runtime, "initializing global: %d", i);

                if(WASM_DEBUG_InitGlobals) ESP_LOGI("WASM3","InitGlobals: init global: %d", i);
                if (g->initExpr)
                {
                    bytes_t start = g->initExpr;
                    
                    if(WASM_DEBUG_InitGlobals) ESP_LOGI("WASM3", "InitGlobals: EvaluateExpression(i64Value: %p, type: %d, start: %p, initExpr: %p, initExprSize: %d", 
                        &g->i64Value, g->type, &start, g->initExpr, g->initExprSize); 
                    
                    result = EvaluateExpression (io_module, & g->i64Value, g->type, & start, g->initExpr + g->initExprSize);

                    if (not result)
                    {
                        // io_module->globalMemory [i] = initValue;
                    }
                    else break;
                }
                else
                {                                                    
                    m3log (runtime, "importing global");
                }
            }            
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    return result;
}


DEBUG_TYPE WASM_DEBUG_INIT_DATA_SEGMENTS = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result InitDataSegments(IM3Memory io_memory, IM3Module io_module)
{
    M3Result result = m3Err_none;
    
    // Verifica che la struttura di memoria sia inizializzata
    _throwif("uninitialized M3Memory structure", !io_memory || io_memory->firm != INIT_FIRM);

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
            if(WASM_DEBUG_INIT_DATA_SEGMENTS) ESP_LOGI("WASM3", "InitDataSegments: add segments up to %d", end_segment);
            if (end_segment > io_memory->num_segments && !AddSegments(io_memory, end_segment)) {
                _throw("failed to allocate memory segment");
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

                if(!io_memory->segments[current_segment]->is_allocated){
                    if((result = InitSegment(io_memory, io_memory->segments[current_segment], true))){
                        _throw(result);
                    }
                }
                
                u8* dest = ((u8*)io_memory->segments[current_segment]->data) + segment_offset;
                //memcpy(dest, segment->data + src_offset, bytes_to_copy);

                if(WASM_DEBUG_INIT_DATA_SEGMENTS) ESP_LOGI("WASM3", "InitDataSegments: m3_memcpy");
                m3_memcpy(io_memory, dest, segment->data + src_offset, bytes_to_copy);
                
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


DEBUG_TYPE WASM_DEBUG_INIT_ELEMENTS = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result  InitElements  (IM3Module io_module)
{
    M3Result result = m3Err_none;
    IM3Memory mem = &io_module->runtime->memory;

    bytes_t bytes = io_module->elementSection;
    cbytes_t end = io_module->elementSectionEnd;

    for (u32 i = 0; i < io_module->numElementSegments; ++i)
    {
        u32 index;
_       (ReadLEB_u32 (mem, & index, & bytes, end));

        if (index == 0)
        {
            i32 offset;
_           (EvaluateExpression (io_module, & offset, c_m3Type_i32, & bytes, end));
            _throwif ("table underflow", offset < 0);

            u32 numElements;
_           (ReadLEB_u32 (mem, & numElements, & bytes, end));

            size_t endElement = (size_t) numElements + offset;
            _throwif ("table overflow", endElement > d_m3MaxSaneTableSize);

            // is there any requirement that elements must be in increasing sequence?
            // make sure the table isn't shrunk.
            if (endElement > io_module->table0Size)
            {
                if(WASM_DEBUG_INIT_ELEMENTS) ESP_LOGI("WASM3", "InitElements: m3_ReallocArray IM3Function");
                //m3_ReallocArray (&io_module->runtime->memory, io_module->table0, IM3Function, endElement);
                io_module->table0 = m3_Def_ReallocArray (IM3Function, io_module->table0, endElement);
                io_module->table0Size = (u32) endElement;

                if(io_module->table0 == NULL){
                    ESP_LOGE("WASM3", "InitElements: m3_ReallocArray IM3Function FAILED");
                }
            }
            _throwifnull(io_module->table0);

            for (u32 e = 0; e < numElements; ++e)
            {
                u32 functionIndex;
_               (ReadLEB_u32 (mem, & functionIndex, & bytes, end));
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
        result = (M3Result) RunCode (function->compiled,  runtime->stack, &runtime->memory, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
        result = (M3Result) RunCode (function->compiled,  runtime->stack, &runtime->memory, d_m3OpDefaultArgs);
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

DEBUG_TYPE WASM_DEBUG_m3_LoadModule =  WASM_DEBUG_ALL || (WASM_DEBUG && false);
// TODO: deal with main + side-modules loading efforcement
M3Result  m3_LoadModule  (IM3Runtime io_runtime, IM3Module io_module)
{
    // Start
    M3Result result = m3Err_none;

    // Debug memory allocation
    multi_heap_info_t info;
    if(WASM_DEBUG_m3_LoadModule){        
        heap_caps_get_info(&info, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        ESP_LOGI("WASM3", "Before LoadModule - Free: %d bytes, Largest block: %d bytes", 
            info.total_free_bytes, info.largest_free_block);
    }

    if(!is_ptr_valid(io_module->runtime) || io_module->runtime->memory.firm != INIT_FIRM){                        
        io_module->runtime = io_runtime;        
    }
    else {        
        if(io_runtime != io_module->runtime){
            ESP_LOGW("WASM3", "m3_LoadModule: m3Err_moduleAlreadyLinked");
            return m3Err_moduleAlreadyLinked;
        }
    }

    if(WASM_DEBUG_m3_LoadModule) ESP_LOGI("WASM3", "Starting InitMemory");
_   (InitMemory (io_runtime, io_module));
    if(WASM_DEBUG_m3_LoadModule) ESP_LOGI("WASM3", "InitMemory completed");

    if(WASM_DEBUG_m3_LoadModule) ESP_LOGI("WASM3", "Starting InitGlobals");
_   (InitGlobals (io_module));
    if(WASM_DEBUG_m3_LoadModule) ESP_LOGI("WASM3", "InitGlobals completed");

    if(WASM_DEBUG_m3_LoadModule) ESP_LOGI("WASM3", "Starting InitDataSegments");
_   (InitDataSegments (&io_runtime->memory, io_module));
    if(WASM_DEBUG_m3_LoadModule) ESP_LOGI("WASM3", "InitDataSegments completed");

    if(WASM_DEBUG_m3_LoadModule) ESP_LOGI("WASM3", "Starting InitElements");
_   (InitElements (io_module));
    if(WASM_DEBUG_m3_LoadModule) ESP_LOGI("WASM3", "InitElements completed");

#if DEBUG
    Module_GenerateNames(io_module);
#endif

    io_module->next = io_runtime->modules;
    io_runtime->modules = io_module;
    return result; // ok

_catch:
    if(WASM_DEBUG_m3_LoadModule){        
        // Log di debug per capire dove è fallito
        heap_caps_get_info(&info, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        ESP_LOGE("WASM3", "LoadModule failed - Free: %d bytes, Largest block: %d bytes", info.total_free_bytes, info.largest_free_block);
    }
    
    ESP_LOGE("WASM3", "LoadModule failed with result: %d", result);
    
    io_module->runtime = NULL;
    return result;
}

IM3Global  m3_FindGlobal  (IM3Module               io_module, const char * const      i_globalName)
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


DEBUG_TYPE WASM_DEBUG_VERBOSE_v_FindFunction = WASM_DEBUG_ALL || (WASM_DEBUG && false);
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

//DEBUG_TYPE WASM_DEBUG_m3_FindFunction = true;
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
    result = (M3Result) RunCode(i_function->compiled, (ptr)(runtime->stack), &runtime->memory, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
    result = (M3Result) RunCode(i_function->compiled, (ptr)(runtime->stack), &runtime->memory, d_m3OpDefaultArgs);
# endif
    ReportNativeStackUsage();

    runtime->lastCalled = result ? NULL : i_function;

    _catch: return result;
}

M3Result m3_Call(IM3Function i_function, uint32_t i_argc, const void* i_argptrs[])
{
    CALL_WATCHDOG

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
    result = (M3Result) RunCode(i_function->compiled, (runtime->stack), &runtime->memory, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
    result = (M3Result) RunCode(i_function->compiled, (runtime->stack), &runtime->memory, d_m3OpDefaultArgs);
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
    result = (M3Result) RunCode(i_function->compiled, (runtime->stack), &runtime->memory, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
    result = (M3Result) RunCode(i_function->compiled, (runtime->stack), &runtime->memory, d_m3OpDefaultArgs);
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


DEBUG_TYPE WASM_DEBUG_AcquireCodePageWithCapacity = WASM_DEBUG_ALL || (WASM_DEBUG && false);
IM3CodePage  AcquireCodePageWithCapacity  (IM3Runtime i_runtime, u32 i_minLineCount)
{
    if(WASM_DEBUG_AcquireCodePageWithCapacity) ESP_LOGI("WASM3", "AcquireCodePageWithCapacity: RemoveCodePageOfCapacity");
    IM3CodePage page = RemoveCodePageOfCapacity (& i_runtime->pagesOpen, i_minLineCount);

    if (not page)
    {
        if(WASM_DEBUG_AcquireCodePageWithCapacity) ESP_LOGI("WASM3", "AcquireCodePageWithCapacity: Environment_AcquireCodePage");
        page = Environment_AcquireCodePage (i_runtime->environment, i_minLineCount);

        if (not page){
            if(WASM_DEBUG_AcquireCodePageWithCapacity) ESP_LOGI("WASM3", "AcquireCodePageWithCapacity: NewCodePage");
            page = NewCodePage (i_runtime, i_minLineCount);
        }

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
                #if d_m3_dump_code_pages
                    dump_code_page (i_codePage, /* startPC: */ NULL);
                #endif
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
                    if (!i_runtime->memory.segments[i]->is_allocated)
                    {
                        //size_t seg_size = (i == i_runtime->memory.num_segments - 1) ? (size - (i * i_runtime->memory.segment_size)) :  i_runtime->memory.segment_size;
                        size_t seg_size = i_runtime->memory.segment_size;

                        i_runtime->memory.segments[i]->data = current_allocator->malloc(seg_size);
                        if (!i_runtime->memory.segments[i]->data)
                        {
                            // Fallimento: libera tutto e ritorna NULL
                            for (size_t j = 0; j < i; j++) {
                                current_allocator->free(i_runtime->memory.segments[j]->data);
                            }
                            current_allocator->free(memory);
                            return NULL;
                        }
                        i_runtime->memory.segments[i]->is_allocated = true;
                        //i_runtime->memory.segments[i]->size = seg_size;
                        memset(i_runtime->memory.segments[i]->data, 0, seg_size);
                    }
                    
                    // Copia i dati dal segmento al blocco contiguo
                    size_t offset = i * i_runtime->memory.segment_size;
                    memcpy(memory + offset, 
                           i_runtime->memory.segments[i]->data,
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

