//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#define M3_IMPLEMENT_ERROR_STRINGS

#include "m3_core.h"
#include "m3_env.h"
#include "m3_config.h"

#include "esp_debug_helpers.h"
#include "esp_heap_caps.h"
#include "esp_try.h"

//#include "m3_compile.h"


void m3_Abort(const char* message) {
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    return m3Err_none;
}

#if d_m3LogTimestamps

#include <time.h>

#define SEC_TO_US(sec) ((sec)*1000000)
#define NS_TO_US(ns)    ((ns)/1000)

static uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp()
{
    if (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    return us - initial_ts;
}

#endif

#if d_m3FixedHeap

static u8 fixedHeap[d_m3FixedHeap];
static u8* fixedHeapPtr = fixedHeap;
static u8* const fixedHeapEnd = fixedHeap + d_m3FixedHeap;
static u8* fixedHeapLast = NULL;

#if d_m3FixedHeapAlign > 1
#   define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#else
#   define HEAP_ALIGN_PTR(P)
#endif

// ex m3_Malloc_Impl etc...

#endif

///
/// General functions
///

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

///
/// Segmented memory implementation
///

void print_memory_info(){
    // Ottenere memoria libera totale e più grande blocco contiguo
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    ESP_LOGI("WASM3", "Total free memory: %d bytes\n", free_heap);
    ESP_LOGI("WASM3", "Greater block: %d bytes\n", largest_block);

    // Per memoria interna (IRAM)
    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ESP_LOGI("WASM3", "Internal free memory: %d bytes\n", free_internal);

    // Per vedere la frammentazione
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    ESP_LOGI("WASM3", "Total free blocks: %d\n", info.total_free_bytes);
    ESP_LOGI("WASM3", "Minimum free memory: %d bytes\n", info.minimum_free_bytes);  
}


const bool DEBUG_MEMORY = false;
const bool INT_MEM_SEGMENTED = false;

void *  m3_Int_CopyMem  (const void * i_from, size_t i_size)
{
    if(DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_CopyMem");
    
    if(INT_MEM_SEGMENTED){
        /*void* ptr_dest;
        m3_memcpy(globalMemory, i_from, ptr_dest, i_size);
        return ptr_dest;*/
    }

    /// Old implementation
    void * ptr = m3_Def_Malloc(i_size);
    if (ptr) {
        memcpy (ptr, i_from, i_size);
    }
    return ptr;
}

// Allocatore di default che usa heap_caps
//static const int WASM_ENABLE_SPI_MEM = 0;
static const int ALLOC_SHIFT_OF = 0; // 4
static const bool WASM_DEBUG_ALLOCS = false;
static const bool CHECK_MEMORY_AVAILABLE = false;
static const bool DEFAULT_ALLOC_ALIGNMENT = false;

void* default_malloc(size_t size) {
    if(WASM_DEBUG_ALLOCS) ESP_LOGI("WASM3", "default_malloc called size: %u", size);

    TRY {
        if(CHECK_MEMORY_AVAILABLE){
            print_memory_info();
        }

        size_t aligned_size = DEFAULT_ALLOC_ALIGNMENT ? (size + 7) & ~7 : size; 

        void* ptr = WASM_ENABLE_SPI_MEM ? heap_caps_malloc(aligned_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM) : NULL;
        if (ptr == NULL) {
            ptr = heap_caps_malloc(aligned_size + ALLOC_SHIFT_OF, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL); // | MALLOC_CAP_INTERNAL           
        }

        if (ptr) {
            memset(ptr, 0, aligned_size + ALLOC_SHIFT_OF);  // Zero-fill con padding
        }

        if(ptr == NULL){
            ESP_LOGE("WASM3", "Failed to allocate memory of size %d", aligned_size);
            esp_backtrace_print(100);
        }

        return ptr;

    } CATCH {
        ESP_LOGE("WASM3", "default_malloc: Exception occurred during malloc: %s", esp_err_to_name(last_error));
        return NULL;
    } 
    END_TRY;
}

static const bool WASM_DEBUG_DEFAULT_FREE = false;
static const bool WAMS_DEFAULT_FREE_CHECK_FREEEABLE = true;
void default_free(void* ptr) {
    if(WASM_DEBUG_ALLOCS) ESP_LOGI("WASM3", "default_free called for %p", ptr);

    TRY {
        if (!ptr) return;
        
        // Logging del puntatore prima della free
        if(WASM_DEBUG_DEFAULT_FREE) ESP_LOGD("WASM3", "Attempting to free memory at %p", ptr);
        
        bool notFreeToFree = false;
        if (WAMS_DEFAULT_FREE_CHECK_FREEEABLE && !is_ptr_freeable(ptr)) {            
            ESP_LOGW("WASM3", "default_free: is_ptr_freeable check failed for pointer");
            backtrace();
            //notFreeToFree = true;
            //return;
        }

        if(notFreeToFree) ESP_LOGW("WASM3", "default_free: theoretically, is_ptr_freeable check failed for pointer");

        if (!ultra_safe_free((void**)&ptr)) {
            ESP_LOGW("WASM3", "Skipped unsafe free operation");
            return;
        }
    } CATCH {
        ESP_LOGE("WASM3", "default_free: Exception occurred during free: %s", esp_err_to_name(last_error));
        //return ESP_FAIL;
    }
    END_TRY;
}

static const bool REALLOC_USE_MALLOC_IF_NEW = false;
void* default_realloc(void* ptr, size_t new_size) {
    if(WASM_DEBUG_ALLOCS) ESP_LOGI("WASM3", "default_realloc called for %p (size: %u)", ptr, new_size);

    size_t aligned_size = DEFAULT_ALLOC_ALIGNMENT ? (new_size + 7) & ~7 : new_size;

    TRY {
        if(!ptr){
            ptr = default_malloc(aligned_size);
            return ptr;
        }

        if(!ultra_safe_ptr_valid(ptr)){
            ESP_LOGW("WASM3", "default_free: is_ptr_freeable check failed for pointer");
            backtrace();
            return;
        }

        // Ottieni la dimensione originale del blocco
        size_t old_size = heap_caps_get_allocated_size(ptr);
        if(WASM_DEBUG_ALLOCS) ESP_LOGI("WASM3", "Original block size: %zu", old_size);

        void* new_ptr = WASM_ENABLE_SPI_MEM ? 
            heap_caps_realloc(ptr, aligned_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM) : NULL;
        
        if (new_ptr == NULL) {
            if(REALLOC_USE_MALLOC_IF_NEW && ptr == NULL){
                new_ptr = default_malloc(aligned_size);
            }
            else {
                new_ptr = heap_caps_realloc(ptr, aligned_size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
            }
        }

        if(new_ptr && aligned_size > old_size) {
            // Azzera solo la nuova porzione allocata
            memset((uint8_t*)new_ptr + old_size, 0, aligned_size - old_size);
            if(WASM_DEBUG_ALLOCS) ESP_LOGI("WASM3", "Zeroed %zu bytes from offset %zu", 
                                          aligned_size - old_size, old_size);
        }

        if (new_ptr == NULL){
            ESP_LOGE("WASM3", "Failed to reallocate memory of size %zu", aligned_size);
        }

        ptr = new_ptr;
        return new_ptr;

    } CATCH {
        ESP_LOGE("WASM3", "default_realloc: Exception occurred during realloc: %s", 
                 esp_err_to_name(last_error));
        return ptr;
    }
    END_TRY;
}

void m3_SetMemoryAllocator(MemoryAllocator* allocator) {
    current_allocator = allocator ? allocator : &default_allocator;
}

const bool WASM_DEBUG_MALLOC_IMPL_BACKTRACE = false;
void* m3_Malloc_Impl(size_t i_size) {
    if(WASM_DEBUG_MALLOC_IMPL_BACKTRACE) esp_backtrace_print(100);

    if (DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Malloc_Impl of size %zu", i_size);

    M3Memory* memory = (M3Memory*)current_allocator->malloc(sizeof(M3Memory));
    if (!memory) {
        ESP_LOGE("WASM3", "Null M3Memory pointer");
        return NULL;
    }

    memory->segment_size = WASM_SEGMENT_SIZE;    
    memory->total_size = i_size;
    memory->num_segments = (i_size + memory->segment_size - 1) / memory->segment_size;
    //memory->point = 0; // Moved to M3MemoryPoint

    ESP_LOGI("WASM3", "m3_Malloc_Imp memory->segment_size = %zu", memory->segment_size);

    // Alloca array di strutture MemorySegment
    memory->segments = (MemorySegment*)current_allocator->malloc(
        memory->num_segments * sizeof(MemorySegment)
    );
    
    if (!memory->segments) {
        current_allocator->free(memory);
        return NULL;
    }

    // Inizializza tutti i segmenti come non allocati
    for (size_t i = 0; i < memory->num_segments; i++) {
        memory->segments[i]->data = NULL;
        memory->segments[i]->is_allocated = false;
        //memory->segment_size = 0; //???
    }

    if (DEBUG_MEMORY) ESP_LOGI("WASM3", "Returning memory pointer");
    //esp_backtrace_print(100); 

    return memory;
}

void m3_Free_Impl(void* io_ptr, bool isMemory) {
    if (DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Free_Impl");

    if (!is_ptr_freeable(&io_ptr) || io_ptr == NULL) return;

    if (isMemory) {
        M3Memory* memory = (M3Memory*)io_ptr;
        
        if (memory->segments) {
            // Libera solo i segmenti effettivamente allocati
            for (size_t i = 0; i < memory->num_segments; i++) {
                if (memory->segments[i]->is_allocated && memory->segments[i]->data) {
                    if(is_ptr_freeable(&memory->segments[i]->data))
                        current_allocator->free(memory->segments[i]->data);
                }
            }

            if(is_ptr_freeable(&memory->segments))
                current_allocator->free(memory->segments);
        }
        
        if(!is_ptr_freeable(&memory)){
            ESP_LOGI("WASM3", "m3_Free_Impl: not safe to free memory");
        }

        //current_allocator->free(memory); // DON'T do it (why?)
    } else {
        current_allocator->free(io_ptr);
    }
}

void* m3_Realloc_Impl(void* i_ptr, size_t i_newSize, size_t i_oldSize) {
    ESP_LOGI("WASM3", "Requesting realloc: old=%zu new=%zu", i_oldSize, i_newSize);

    //if (!i_ptr) return m3_Malloc_Impl(i_newSize); // this is kinda of stupid

    if(i_ptr){
        if (validate_pointer(i_ptr, sizeof(M3Memory)) == PTR_CHECK_OK) {
            M3Memory* memory = (M3Memory*)i_ptr;
            size_t new_num_segments = (i_newSize + memory->segment_size - 1) / memory->segment_size;

            // Riallocare l'array dei segmenti se necessario
            if (new_num_segments != memory->num_segments) {
                MemorySegment** new_segments = current_allocator->realloc(
                    memory->segments,
                    new_num_segments * sizeof(MemorySegment*)
                );

                if (!new_segments) return NULL;
                
                // Inizializza i nuovi segmenti se stiamo crescendo
                for (size_t i = memory->num_segments; i < new_num_segments; i++) {
                    new_segments[i]->data = NULL;
                    new_segments[i]->is_allocated = false;
                    //new_segments[i].size = 0;
                }

                // Libera i segmenti in eccesso se stiamo riducendo
                for (size_t i = new_num_segments; i < memory->num_segments; i++) {
                    if (new_segments[i]->is_allocated && new_segments[i]->data) {
                        current_allocator->free(new_segments[i]->data);
                    }
                }

                memory->segments = new_segments;
                memory->num_segments = new_num_segments;
                memory->total_size = i_newSize;
            }

            return i_ptr;
        }
        else {
            if(is_ptr_freeable(i_ptr)) current_allocator->free(i_ptr);
            return current_allocator->realloc(i_ptr, i_newSize);
        }        
    }

    return m3_Malloc_Impl(i_newSize);
}

void* m3_CopyMem(const void* i_from, size_t i_size) {
    if (DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_CopyMem");
    if (!i_from) return NULL;

    M3Memory* src_memory = (M3Memory*)i_from;
    M3Memory* dst_memory = (M3Memory*)m3_Malloc_Impl(i_size);
    
    if (!dst_memory) return NULL;

    // Copia i metadati
    dst_memory->segment_size = src_memory->segment_size;
    dst_memory->total_size = i_size;

    // Copia solo i segmenti che sono effettivamente allocati
    for (size_t i = 0; i < src_memory->num_segments && i < dst_memory->num_segments; i++) {
        if (src_memory->segments[i]->is_allocated) {
            // Alloca e copia il segmento
            if (allocate_segment(dst_memory, i)) {
                size_t copy_size = M3_MIN(
                    src_memory->segment_size,
                    dst_memory->segment_size
                );
                memcpy(dst_memory->segments[i]->data,
                       src_memory->segments[i]->data,
                       copy_size);
            }
        }
    }

    return dst_memory;
}

//--------------------------------------------------------------------------------------------

#if d_m3LogNativeStack

static size_t stack_start;
static size_t stack_end;

void        m3StackCheckInit ()
{
    char stack;
    stack_end = stack_start = (size_t)&stack;
}

void        m3StackCheck ()
{
    char stack;
    size_t addr = (size_t)&stack;

    size_t stackEnd = stack_end;
    stack_end = M3_MIN (stack_end, addr);

//    if (stackEnd != stack_end)
//        printf ("maxStack: %ld\n", m3StackGetMax ());
}

int      m3StackGetMax  ()
{
    return stack_start - stack_end;
}

#endif

//--------------------------------------------------------------------------------------------

M3Result NormalizeType (u8 * o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    if (type == 0x40)
        type = c_m3Type_none;
    else if (type < c_m3Type_i32 or type > c_m3Type_f64)
        result = m3Err_invalidTypeId;

    * o_type = type;

    return result;
}


bool  IsFpType  (u8 i_m3Type)
{
    return (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool  IsIntType  (u8 i_m3Type)
{
    return (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool  Is64BitType  (u8 i_m3Type)
{
    if (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64)
        return true;
    else if (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none)
        return false;
    else
        return (sizeof (voidptr_t) == 8); // all other cases are pointers
}

u32  SizeOfType  (u8 i_m3Type)
{
    if (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32)
        return sizeof (i32);

    return sizeof (i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result  Read_u64  (u64 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;
    ptr += sizeof (u64);

    if (ptr <= i_end)
    {
        memcpy(o_value, * io_bytes, sizeof(u64));
        M3_BSWAP_u64(*o_value);
        * io_bytes = ptr;
        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}


M3Result  Read_u32  (u32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;
    ptr += sizeof (u32);

    if (ptr <= i_end)
    {
        memcpy(o_value, * io_bytes, sizeof(u32));
        M3_BSWAP_u32(*o_value);
        * io_bytes = ptr;
        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}

#if d_m3ImplementFloat

M3Result  Read_f64  (f64 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;
    ptr += sizeof (f64);

    if (ptr <= i_end)
    {
        memcpy(o_value, * io_bytes, sizeof(f64));
        M3_BSWAP_f64(*o_value);
        * io_bytes = ptr;
        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}


M3Result  Read_f32  (f32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;
    ptr += sizeof (f32);

    if (ptr <= i_end)
    {
        memcpy(o_value, * io_bytes, sizeof(f32));
        M3_BSWAP_f32(*o_value);
        * io_bytes = ptr;
        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}

#endif

M3Result  Read_u8  (u8 * o_value, bytes_t  * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;

    if (ptr < i_end)
    {
        * o_value = * ptr;
        * io_bytes = ptr + 1;

        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}

M3Result  Read_opcode  (m3opcode_t * o_value, bytes_t  * io_bytes, cbytes_t i_end)
{
    const u8 * ptr = * io_bytes;

    if (ptr < i_end)
    {
        m3opcode_t opcode = * ptr++;

#if d_m3CascadedOpcodes == 0
        if (M3_UNLIKELY(opcode == c_waOp_extended))
        {
            if (ptr < i_end)
            {
                opcode = (opcode << 8) | (* ptr++);
            }
            else return m3Err_wasmUnderrun;
        }
#endif
        * o_value = opcode;
        * io_bytes = ptr;

        return m3Err_none;
    }
    else return m3Err_wasmUnderrun;
}


M3Result  ReadLebUnsigned  (u64 * o_value, u32 i_maxNumBits, bytes_t * io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 value = 0;

    u32 shift = 0;
    const u8 * ptr = * io_bytes;

    while (ptr < i_end)
    {
        u64 byte = * (ptr++);

        value |= ((byte & 0x7f) << shift);
        shift += 7;

        if ((byte & 0x80) == 0)
        {
            result = m3Err_none;
            break;
        }

        if (shift >= i_maxNumBits)
        {
            result = m3Err_lebOverflow;
            break;
        }
    }

    * o_value = value;
    * io_bytes = ptr;

    return result;
}


M3Result  ReadLebSigned  (i64 * o_value, u32 i_maxNumBits, bytes_t * io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 value = 0;

    u32 shift = 0;
    const u8 * ptr = * io_bytes;

    while (ptr < i_end)
    {
        u64 byte = * (ptr++);

        value |= ((byte & 0x7f) << shift);
        shift += 7;

        if ((byte & 0x80) == 0)
        {
            result = m3Err_none;

            if ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                value |= (~extend << shift);
            }

            break;
        }

        if (shift >= i_maxNumBits)
        {
            result = m3Err_lebOverflow;
            break;
        }
    }

    * o_value = value;
    * io_bytes = ptr;

    return result;
}


M3Result  ReadLEB_u32  (u32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    u64 value;
    M3Result result = ReadLebUnsigned (& value, 32, io_bytes, i_end);
    * o_value = (u32) value;

    return result;
}


M3Result  ReadLEB_u7  (u8 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    u64 value;
    M3Result result = ReadLebUnsigned (& value, 7, io_bytes, i_end);
    * o_value = (u8) value;

    return result;
}


M3Result  ReadLEB_i7  (i8 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    i64 value;
    M3Result result = ReadLebSigned (& value, 7, io_bytes, i_end);
    * o_value = (i8) value;

    return result;
}


M3Result  ReadLEB_i32  (i32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    i64 value;
    M3Result result = ReadLebSigned (& value, 32, io_bytes, i_end);
    * o_value = (i32) value;

    return result;
}


M3Result  ReadLEB_i64  (i64 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    i64 value;
    M3Result result = ReadLebSigned (& value, 64, io_bytes, i_end);
    * o_value = value;

    return result;
}


M3Result  Read_utf8  (cstr_t * o_utf8, bytes_t * io_bytes, cbytes_t i_end)
{
    *o_utf8 = NULL;

    u32 utf8Length;
    M3Result result = ReadLEB_u32 (& utf8Length, io_bytes, i_end);

    if (not result)
    {
        if (utf8Length <= d_m3MaxSaneUtf8Length)
        {
            const u8 * ptr = * io_bytes;
            const u8 * end = ptr + utf8Length;

            if (end <= i_end)
            {
                char * utf8 = (char *)m3_Def_Malloc (utf8Length + 1);

                if (utf8)
                {
                    memcpy (utf8, ptr, utf8Length);
                    utf8 [utf8Length] = 0;
                    * o_utf8 = utf8;
                }

                * io_bytes = end;
            }
            else result = m3Err_wasmUnderrun;
        }
        else result = m3Err_missingUTF8;
    }

    return result;
}

#if d_m3RecordBacktraces
u32  FindModuleOffset  (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;
    bool pageFound = false;

    while (curr)
    {
        if (ContainsPC (curr, i_pc))
        {
            pageFound = true;
            break;
        }
        curr = curr->info.next;
    }

    if (!pageFound)
    {
        curr = i_runtime->pagesFull;
        while (curr)
        {
            if (ContainsPC (curr, i_pc))
            {
                pageFound = true;
                break;
            }
            curr = curr->info.next;
        }
    }

    if (pageFound)
    {
        u32 result = 0;

        bool pcFound = MapPCToOffset (curr, i_pc, & result);
                                                                                d_m3Assert (pcFound);

        return result;
    }
    else return 0;
}


void  PushBacktraceFrame  (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    if (M3_UNLIKELY (io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED))
        return;

    M3BacktraceFrame * newFrame = m3_AllocStruct(M3BacktraceFrame);

    if (!newFrame)
    {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        return;
    }

    newFrame->moduleOffset = FindModuleOffset (io_runtime, i_pc);

    if (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame)
        io_runtime->backtrace.frames = newFrame;
    else
        io_runtime->backtrace.lastFrame->next = newFrame;
    io_runtime->backtrace.lastFrame = newFrame;
}


void  FillBacktraceFunctionInfo  (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    if (M3_UNLIKELY (io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED))
        return;

    if (!io_runtime->backtrace.lastFrame)
        return;

    io_runtime->backtrace.lastFrame->function = i_function;
}


void  ClearBacktrace  (IM3Runtime io_runtime)
{
    M3BacktraceFrame * currentFrame = io_runtime->backtrace.frames;
    while (currentFrame)
    {
        M3BacktraceFrame * nextFrame = currentFrame->next;
        m3_Def_Free (currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames = NULL;
    io_runtime->backtrace.lastFrame = NULL;
}
#endif // d_m3RecordBacktraces
