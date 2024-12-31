//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_esp_try.h"
#include "m3_exec_defs.h"
#include "wasm3.h"
#include "wasm3_defs.h"

#define M3_IMPLEMENT_ERROR_STRINGS

#include "m3_core.h"
#include "m3_env.h"


//#include "m3_compile.h"

void m3_Abort(const char* message) {
#if DEBUG
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

    size_t spiram_internal = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_LOGI("WASM3", "SPIRAM free memory: %d bytes\n", spiram_internal);

    // Per vedere la frammentazione
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    ESP_LOGI("WASM3", "Total free blocks: %d\n", info.total_free_bytes);
    ESP_LOGI("WASM3", "Minimum free memory: %d bytes\n", info.minimum_free_bytes);  
}


DEBUG_TYPE WASM_DEBUG_MEMORY = DEBUG_MEMORY || WASM_DEBUG_ALL || (WASM_DEBUG && false);
const bool WASM_INT_MEM_SEGMENTED = false; 
// Just used in function costants
void *  m3_Int_CopyMem  (const void * i_from, size_t i_size)
{
    if(WASM_DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_CopyMem");
    
    if(WASM_INT_MEM_SEGMENTED && false){
        void* ptr_dest;
        //m3_memcpy(globalMemory, i_from, ptr_dest, i_size); // it couldn't work due to the lack of memory reference
        return ptr_dest;
    }
    else {
        /// Old implementation
        void * ptr = m3_Def_Malloc(i_size);
        if (ptr) {
            memcpy (ptr, i_from, i_size);
        }    
        return ptr;
    }
}

// Allocatore di default che usa heap_caps
//static const int WASM_ENABLE_SPI_MEM = 0;
static const int ALLOC_SHIFT_OF = 0; // 4
DEBUG_TYPE WASM_DEBUG_DEFAULT_ALLOCS = WASM_DEBUG_ALL || (WASM_DEBUG && false);
static const bool PRINT_CHECK_RAM_MEMORY_AVAILABLE = false;
static const bool DEFAULT_ALLOC_ALIGNMENT = true;

bool check_memory_available_bySize(size_t required_size) {
    const char * TAG = "WASM3";

    #if WASM_ENABLE_SPI_MEM
        // Check both internal and PSRAM memory
        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        size_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        size_t largest_spiram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        if(WASM_DEBUG_DEFAULT_ALLOCS){
            ESP_LOGD(TAG, "Free internal: %zu, largest block: %zu", free_internal, largest_internal);
            ESP_LOGD(TAG, "Free SPIRAM: %zu, largest block: %zu", free_spiram, largest_spiram);
        }

        // Check if allocation can fit in either memory type
        return (largest_spiram >= required_size); // (largest_internal >= required_size) || 
    #else
        // Only check internal memory
        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        size_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

        if(WASM_DEBUG_DEFAULT_ALLOCS) ESP_LOGD(TAG, "Free internal: %zu, largest block: %zu", free_internal, largest_internal);

        return largest_internal >= required_size;
    #endif
}

static u32 call_default_alloc_cycle =  0;
void call_default_alloc(){
    if(call_default_alloc_cycle++ % 5 == 0) { CALL_WATCHDOG }
}

#define ALLOC_ON_EXEC 0

#if ALLOC_ON_EXEC
const uint32_t default_alloc_caps = MALLOC_CAP_8BIT | MALLOC_CAP_EXEC | MALLOC_CAP_CACHE_ALIGNED; // MALLOC_CAP_32BIT 
#else 
const uint32_t default_alloc_caps = MALLOC_CAP_8BIT;
#endif

void* default_malloc(size_t size) {
    if(WASM_DEBUG_DEFAULT_ALLOCS) ESP_LOGI("WASM3", "default_malloc called size: %u", size);

    call_default_alloc();

    TRY {
        if(PRINT_CHECK_RAM_MEMORY_AVAILABLE){
            print_memory_info();
        }

        if(!check_memory_available_bySize(size)){
            ESP_LOGE("WASM3", "No memory available (size: %zu)", size);
            LOG_FLUSH;
            backtrace();
            return ERROR_POINTER;
        }

        size_t aligned_size = DEFAULT_ALLOC_ALIGNMENT ? (size + 7) & ~7 : size; 
        aligned_size += ALLOC_SHIFT_OF;

        void* ptr = WASM_ENABLE_SPI_MEM ? heap_caps_malloc(aligned_size, default_alloc_caps | MALLOC_CAP_SPIRAM) : NULL;
        if (ptr == NULL || ptr == ERROR_POINTER) {
            ptr = heap_caps_malloc(aligned_size, default_alloc_caps | MALLOC_CAP_INTERNAL); // | MALLOC_CAP_INTERNAL           
        }

        if(ptr == NULL || ptr == ERROR_POINTER){
            ESP_LOGE("WASM3", "Failed to allocate memory of size %zu", aligned_size);
            print_memory_info();
            //esp_backtrace_print(100);
            return NULL;
        }

        if(WASM_DEBUG_DEFAULT_ALLOCS) ESP_LOGI("WASM3", "default_malloc resulting ptr: %p", ptr);

        #if ALLOC_ON_EXEC
        // Sincronizza la cache prima della scrittura        
        esp_cache_msync(ptr, aligned_size, ESP_CACHE_MSYNC_FLAG_INVALIDATE);
        #endif 

        memset(ptr, 0, aligned_size);  // Zero-fill con padding  

        #if ALLOC_ON_EXEC
        // Sincronizza la cache dopo la scrittura
        esp_cache_msync(ptr, aligned_size, ESP_CACHE_MSYNC_FLAG_INVALIDATE | ESP_CACHE_MSYNC_FLAG_TYPE_INST);
        #endif

        if(WASM_DEBUG_DEFAULT_ALLOCS) ESP_LOGI("WASM3", "default_malloc resulting ptr after memset: %p", ptr);

        return ptr;

    } CATCH {
        ESP_LOGE("WASM3", "default_malloc: Exception occurred during malloc: %s", esp_err_to_name(last_error));
        return NULL;
    } 
    END_TRY;
}

DEBUG_TYPE WASM_DEBUG_DEFAULT_FREE = WASM_DEBUG_ALL || (WASM_DEBUG && false);
const bool WAMS_DEFAULT_FREE_CHECK_FREEEABLE = true;
const bool WASM_default_free_WARNS_NOT_FREEABLE = false;
void default_free(void* ptr) {
    if(WASM_DEBUG_DEFAULT_ALLOCS) ESP_LOGI("WASM3", "default_free called for %p", ptr);

    call_default_alloc();

    TRY {
        if (!ptr || ptr == ERROR_POINTER) return;
        
        // Logging del puntatore prima della free
        if(WASM_DEBUG_DEFAULT_FREE) ESP_LOGD("WASM3", "Attempting to free memory at %p", ptr);
        
        bool notFreeToFree = false;
        if (WAMS_DEFAULT_FREE_CHECK_FREEEABLE && !is_ptr_freeable(ptr)) {            
            if(WASM_default_free_WARNS_NOT_FREEABLE) ESP_LOGW("WASM3", "default_free: is_ptr_freeable check failed for pointer");
            //backtrace();
            //notFreeToFree = true;
            return;
        }

        if(notFreeToFree) ESP_LOGW("WASM3", "default_free: theoretically, is_ptr_freeable check failed for pointer");

        if (!ultra_safe_free((void**)&ptr)) {
            if(WASM_default_free_WARNS_NOT_FREEABLE) ESP_LOGW("WASM3", "Skipped unsafe free operation");
            return;
        }
    } CATCH {
        ESP_LOGE("WASM3", "default_free: Exception occurred during free: %s", esp_err_to_name(last_error));
        //return ESP_FAIL;
    }
    END_TRY;
}

static const bool REALLOC_USE_MALLOC_IF_NEW = true;
void* default_realloc(void* ptr, size_t new_size) {
    if(WASM_DEBUG_DEFAULT_ALLOCS) ESP_LOGI("WASM3", "default_realloc called for %p (size: %u)", ptr, new_size);

    call_default_alloc();

    size_t aligned_size = DEFAULT_ALLOC_ALIGNMENT ? (new_size + 7) & ~7 : new_size;
    aligned_size += ALLOC_SHIFT_OF;

    TRY {
        if(!ptr){
            ptr = default_malloc(aligned_size);
            return ptr;
        }

        if(!check_memory_available_bySize(new_size)){
            ESP_LOGE("WASM3", "No memory available (size: %u)", new_size);
            ESP_LOGE("WASM3", "No memory available (size: %u)", new_size);
            backtrace();
            return ERROR_POINTER;
        }

        if(!ultra_safe_ptr_valid(ptr)){
            ESP_LOGW("WASM3", "default_realloc: is_ptr_freeable check failed for pointer");
            backtrace();
            return ERROR_POINTER;
        }

        // Ottieni la dimensione originale del blocco
        size_t old_size = heap_caps_get_allocated_size(ptr);
        if(WASM_DEBUG_DEFAULT_ALLOCS) ESP_LOGI("WASM3", "Original block size: %zu", old_size);

        void* new_ptr = NULL;

        if(REALLOC_USE_MALLOC_IF_NEW && ptr == NULL){
                new_ptr = default_malloc(aligned_size);
        }

        new_ptr = WASM_ENABLE_SPI_MEM ? 
            heap_caps_realloc(ptr, aligned_size, default_alloc_caps | MALLOC_CAP_SPIRAM) : NULL;
        
        if (new_ptr == NULL) {
            new_ptr = heap_caps_realloc(ptr, aligned_size, default_alloc_caps | MALLOC_CAP_INTERNAL);            
        }

        if(new_ptr && aligned_size > old_size) {
            #if ALLOC_ON_EXEC
            // Sincronizza la cache prima della scrittura
            esp_cache_msync(ptr, aligned_size, ESP_CACHE_MSYNC_FLAG_INVALIDATE);
            #endif

            // Azzera solo la nuova porzione allocata
            memset((uint8_t*)new_ptr + old_size, 0, aligned_size - old_size);
            if(WASM_DEBUG_DEFAULT_ALLOCS) ESP_LOGI("WASM3", "Zeroed %zu bytes from offset %zu",  aligned_size - old_size, old_size);

            #if ALLOC_ON_EXEC
            // Sincronizza la cache dopo la scrittura
            esp_cache_msync(ptr, aligned_size, ESP_CACHE_MSYNC_FLAG_INVALIDATE | ESP_CACHE_MSYNC_FLAG_TYPE_INST);
            #endif
        }

        if (new_ptr == NULL){
            ESP_LOGE("WASM3", "Failed to reallocate memory of size %zu", aligned_size);
        }        

        ptr = &new_ptr;

        if(ptr == NULL){
            return ERROR_POINTER;
        }

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

DEBUG_TYPE WASM_DEBUG_MALLOC_IMPL_BACKTRACE = WASM_DEBUG_ALL || (WASM_DEBUG && false);
void* m3_Malloc_Impl(size_t i_size) {
    if(WASM_DEBUG_MALLOC_IMPL_BACKTRACE) esp_backtrace_print(100);

    if (WASM_DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Malloc_Impl of size %zu", i_size);

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

    if (WASM_DEBUG_MEMORY) ESP_LOGI("WASM3", "Returning memory pointer");
    //esp_backtrace_print(100); 

    return memory;
}

void m3_Free_Impl(void* io_ptr, bool isMemory) {
    if (WASM_DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Free_Impl");

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
    //todo: check if segmented offset
    if (WASM_DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_CopyMem");
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

DEBUG_TYPE WASM_DEBUG_SizeOfType = WASM_DEBUG_ALL || (WASM_DEBUG && false);
u32  SizeOfType  (u8 i_m3Type)
{
    if(WASM_DEBUG_SizeOfType) ESP_LOGI("WASM3", "SizeOfType called with i_m3Type: %d", i_m3Type);

    u32 res;
    if (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32){
        res = sizeof (i32);
        if(WASM_DEBUG_SizeOfType){ 
            ESP_LOGI("WASM3", "SizeOfType res: %d", res); 
            LOG_FLUSH;
        }
        return res;
    }

    res = sizeof (i64);
    if(WASM_DEBUG_SizeOfType){ 
        ESP_LOGI("WASM3", "SizeOfType res: %d", res); 
        LOG_FLUSH;
    }
    return res;
}

////
////
////

//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------
const bool WASM_READ_BACKTRACE_WASMUNDERRUN = true;
const bool WASM_READ_IGNORE_END = false;

DEBUG_TYPE WASM_DEBUG_READ_RESOLVE_POINTER = WASM_DEBUG_ALL || (WASM_DEBUG && false);
DEBUG_TYPE WASM_DEBUG_READ_CHECKWASMUNDERRUN = WASM_DEBUG_ALL || (WASM_DEBUG && false);
DEBUG_TYPE WASM_DEBUG_Read = WASM_DEBUG_ALL || (WASM_DEBUG && false);

void __read_checkWasmUnderrun(mos pos, mos end){
    if(WASM_DEBUG_READ_CHECKWASMUNDERRUN){
        ESP_LOGE("WASM3", "m3Err_wasmUnderrun (pos=%x, end=%x)", pos, end);
        LOG_FLUSH;
        backtrace();
    }
}

M3Result Read_u64(IM3Memory memory, u64* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if(WASM_DEBUG_Read) ESP_LOGI("WASM3", "Read_u64: o_value=%p, io_bytes=%p", o_value, io_bytes);

    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u64* source_ptr;
    bytes_t dest_ptr;
    
    if (memory) {
        //source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        //dest_ptr = (u8*)m3_ResolvePointer(memory, o_value);

        source_ptr = MEMACCESS(const u64*, memory, io_bytes);
        dest_ptr = MEMPOINT(bytes_t, memory, o_value);

        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (u64*)*io_bytes;
        dest_ptr = (u8*)o_value;
    }
    
    // Calculate offset separately
    const size_t offset = sizeof(u64);
    bytes_t check_ptr = (bytes_t)source_ptr + offset;
    cbytes_t end = MEMPOINT(cbytes_t, memory, i_end);

    if ((void*)check_ptr > (void*)end){
        __read_checkWasmUnderrun(CAST_PTR check_ptr, CAST_PTR end);
        return m3Err_wasmUnderrun;
    }

    MEMACCESS(u64, memory, dest_ptr) = * source_ptr;
    MEMACCESS(bytes_t, memory, io_bytes) = check_ptr;
    M3_BSWAP_u64(*(u64*)dest_ptr);
    
    return m3Err_none;
}

M3Result Read_u32(IM3Memory memory, u32* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if(WASM_DEBUG_Read) ESP_LOGI("WASM3", "Read_u32: o_value=%p, io_bytes=%p", o_value, io_bytes);

    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u32* source_ptr;
    bytes_t dest_ptr;
    
    if (memory) {
        source_ptr = MEMACCESS(const u32*, memory, io_bytes);
        dest_ptr = MEMPOINT(bytes_t, memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (u32*)*io_bytes;
        dest_ptr = (bytes_t)o_value;
    }
    
    // Calculate offset separately
    const size_t offset = sizeof(u32);
    const bytes_t check_ptr = (bytes_t)source_ptr + offset;
    cbytes_t end = MEMPOINT(cbytes_t, memory, i_end);

    if ((void*)check_ptr > (void*)end){
        __read_checkWasmUnderrun(CAST_PTR check_ptr, CAST_PTR end);
        return m3Err_wasmUnderrun;
    }

    if(WASM_DEBUG_Read) ESP_LOGW("WASM3", "source_ptr: %p, *source_ptr: %d", source_ptr, *source_ptr);

    MEMACCESS(u32, memory, dest_ptr) = * source_ptr;
    MEMACCESS(bytes_t, memory, io_bytes) = check_ptr;
    M3_BSWAP_u32(*(u32*)dest_ptr);

    return m3Err_none;
}

#if d_m3ImplementFloat
M3Result Read_f64(IM3Memory memory, f64* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if(WASM_DEBUG_Read) ESP_LOGI("WASM3", "Read_f64: o_value=%p, io_bytes=%p", o_value, io_bytes);

    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const f64* source_ptr;
    bytes_t dest_ptr;
    
    if (memory) {
        source_ptr = MEMACCESS(const f64*, memory, io_bytes);
        dest_ptr = MEMPOINT(bytes_t, memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (f64*)*io_bytes;
        dest_ptr = (bytes_t)o_value;
    }
    
    const size_t offset = sizeof(f64);
    const bytes_t check_ptr = (bytes_t)source_ptr + offset;
    cbytes_t end = MEMPOINT(cbytes_t, memory, i_end);
    
    if ((void*)check_ptr > (void*)end){
        __read_checkWasmUnderrun(CAST_PTR check_ptr, CAST_PTR i_end);
        return m3Err_wasmUnderrun;
    }

    MEMACCESS(f64, memory, dest_ptr) = * source_ptr;
    MEMACCESS(bytes_t, memory, io_bytes) = check_ptr;
    M3_BSWAP_f64(*(f64*)dest_ptr);

    return m3Err_none;
}

M3Result Read_f32(IM3Memory memory, f32* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if(WASM_DEBUG_Read) ESP_LOGI("WASM3", "Read_f32: o_value=%p, io_bytes=%p", o_value, io_bytes);

    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const f32* source_ptr;
    bytes_t dest_ptr;
    
    if (memory) {
        source_ptr = MEMACCESS(const f32*, memory, io_bytes);
        dest_ptr = MEMPOINT(bytes_t, memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (f32*)*io_bytes;
        dest_ptr = (bytes_t)o_value;
    }
    
    const size_t offset = sizeof(f32);
    const u8* check_ptr = (bytes_t)source_ptr + offset;
    cbytes_t end = MEMPOINT(cbytes_t, memory, i_end);

    if ((void*)check_ptr > (void*)end){
        __read_checkWasmUnderrun(CAST_PTR check_ptr, CAST_PTR end);
        return m3Err_wasmUnderrun;
    }

    MEMACCESS(f32, memory, dest_ptr) = * source_ptr;
    MEMACCESS(bytes_t, memory, io_bytes) = check_ptr;
    M3_BSWAP_f32(*(f32*)dest_ptr);

    return m3Err_none;
}
#endif

M3Result Read_u8(IM3Memory memory, u8* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if(WASM_DEBUG_Read) ESP_LOGI("WASM3", "Read_u8: o_value=%p, io_bytes=%p", o_value, io_bytes);

    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    bytes_t dest_ptr;
    
    if (memory) {
        source_ptr = MEMACCESS(const u8*, memory, io_bytes);
        dest_ptr = MEMPOINT(bytes_t, memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = (bytes_t)o_value;
    }
    
    const size_t offset = sizeof(u8);
    const bytes_t check_ptr = (bytes_t)source_ptr + offset;
    cbytes_t end = MEMPOINT(cbytes_t, memory, i_end);
    
    if ((void*)check_ptr > (void*)end){
        __read_checkWasmUnderrun(CAST_PTR check_ptr, CAST_PTR end);
        return m3Err_wasmUnderrun;
    }
    
    MEMACCESS(u8, memory, dest_ptr) = * source_ptr;
    MEMACCESS(bytes_t, memory, io_bytes) = check_ptr;

    return m3Err_none;
}

M3Result Read_opcode(IM3Memory memory, m3opcode_t* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if(WASM_DEBUG_Read) ESP_LOGI("WASM3", "Read_opcode: o_value=%p, io_bytes=%p", o_value, io_bytes);

    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    bytes_t dest_ptr;
    
    if (memory) {
        source_ptr = MEMACCESS(u8*, memory, io_bytes);
        dest_ptr = MEMPOINT(bytes_t, memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = (bytes_t)o_value;
    }

    size_t offset = 1; // u8 at time
    bytes_t check_ptr = (bytes_t)source_ptr + offset;

    m3opcode_t opcode;  

    cbytes_t end = MEMPOINT(cbytes_t, memory, i_end);
    if ((void*)check_ptr > (void*)end){
        __read_checkWasmUnderrun(CAST_PTR check_ptr, CAST_PTR end);
        return m3Err_wasmUnderrun;
    }

    opcode = * source_ptr;

    #if d_m3CascadedOpcodes == 0
        if (M3_UNLIKELY(opcode == c_waOp_extended)) {            
            if ((void*)(check_ptr + offset) > (void*)end){
                __read_checkWasmUnderrun(CAST_PTR (check_ptr + offset), CAST_PTR end);
                return m3Err_wasmUnderrun;
            }

            u8 extended_byte = *check_ptr++;            
            opcode = (opcode << 8) | extended_byte;
        }
    #endif    

    MEMACCESS(m3opcode_t, memory, dest_ptr) = opcode;
    MEMACCESS(const u8*, memory, io_bytes) = check_ptr;
    return m3Err_none;
}

M3Result ReadLebUnsigned(IM3Memory memory, u64* o_value, u32 i_maxNumBits, bytes_t* io_bytes, cbytes_t i_end) {
    if(WASM_DEBUG_Read) ESP_LOGI("WASM3", "ReadLebUnsigned: o_value=%p, io_bytes=%p", o_value, io_bytes);

    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    bytes_t dest_ptr;
    
    if (memory) {
        source_ptr = MEMACCESS(const u8*, memory, io_bytes);
        dest_ptr = MEMPOINT(bytes_t, memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = (bytes_t)o_value;
    }

    u64 value = 0;
    u32 shift = 0;
    bytes_t check_ptr = source_ptr;
    M3Result result = m3Err_none;

    cbytes_t end = MEMPOINT(cbytes_t, memory, i_end);
    if(check_ptr > end) {
        result = m3Err_wasmUnderrun;
        ESP_LOGW("WASM3", "ReadLebUnsinged: check_ptr > end (%p > %p)", check_ptr, end);
    }

    while (check_ptr <= end) {
        u64 byte = MEMACCESS(u64, memory, check_ptr);
        check_ptr++;

        value |= ((byte & 0x7f) << shift);
        shift += 7;

        if ((byte & 0x80) == 0) {
            result = m3Err_none;
            break;
        }

        if (shift >= i_maxNumBits) {
            result = m3Err_lebOverflow;
            break;
        }                  
    }
    
    MEMACCESS(u64, memory, dest_ptr) = value;
    MEMACCESS(bytes_t, memory, io_bytes) = check_ptr;

    if (result != NULL && result == m3Err_wasmUnderrun) {
        ESP_LOGW("WASM3", "ReadLebUnsigned: underrun %p == %p", result, m3Err_wasmUnderrun);
        __read_checkWasmUnderrun(CAST_PTR (check_ptr + 1), CAST_PTR i_end);
    }

    return result;
}

M3Result ReadLebSigned(IM3Memory memory, i64* o_value, u32 i_maxNumBits, bytes_t* io_bytes, cbytes_t i_end) {
    if(WASM_DEBUG_Read) ESP_LOGI("WASM3", "ReadLebSigned: o_value=%p, io_bytes=%p", o_value, io_bytes);

    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    bytes_t dest_ptr;
    
    if (memory) {
        source_ptr = MEMACCESS(const u8*, memory, io_bytes);
        dest_ptr = MEMPOINT(bytes_t, memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = o_value;
    }

    i64 value = 0;
    u32 shift = 0;
    bytes_t check_ptr = source_ptr;
    M3Result result = m3Err_none;

    cbytes_t end = MEMPOINT(cbytes_t, memory, i_end);
    if(check_ptr > end) {
        result = m3Err_wasmUnderrun;
    }

    while (check_ptr <= end) {
        u64 byte = MEMACCESS(u64, memory, check_ptr);
        check_ptr++;

        value |= ((byte & 0x7f) << shift);
        shift += 7;

        if ((byte & 0x80) == 0) {
            if ((byte & 0x40) && (shift < 64)) {
                u64 extend = 0;
                value |= (~extend << shift);
            }
            result = m3Err_none;
            break;
        }

        if (shift >= i_maxNumBits) {
            result = m3Err_lebOverflow;
            break;
        }        
    }

    MEMACCESS(i64, memory, dest_ptr) = value;
    MEMACCESS(bytes_t, memory, io_bytes) = check_ptr;

    if (result != NULL && result == m3Err_wasmUnderrun) {
        ESP_LOGW("WASM3", "ReadLebUnsigned: underrun %p == %p", result, m3Err_wasmUnderrun);
        __read_checkWasmUnderrun(CAST_PTR (check_ptr + 1), CAST_PTR i_end);
    }

    return result;
}

///
///
///

M3Result ReadLEB_u32(IM3Memory memory, u32* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!o_value) return m3Err_malformedData;

    u64 value;
    M3Result result = ReadLebUnsigned(memory, &value, 32, io_bytes, i_end);
    *o_value = (u32)value;
    return result;
}

DEBUG_TYPE WASM_DEBUG_ReadLEB_ptr = WASM_DEBUG_ALL || (WASM_DEBUG && false);
M3Result ReadLEB_ptr(IM3Memory memory, m3stack_t o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!o_value) return m3Err_malformedData;
    
    if(WASM_DEBUG_ReadLEB_ptr) ESP_LOGI("WASM3", "ReadLEB_ptr at %d bits (%p <= %p) end %p", (32*BITS_MUL), o_value, io_bytes, i_end);

    u64 value;
    M3Result result = ReadLebUnsigned(memory, &value, 32*BITS_MUL, io_bytes, i_end);
    *o_value = (m3slot_t)value;

    if(WASM_DEBUG_ReadLEB_ptr) ESP_LOGI("WASM3", "ReadLEB_ptr: result: %lld", value);

    return result;
}

M3Result ReadLEB_u7(IM3Memory memory, u8* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!o_value) return m3Err_malformedData;

    u64 value;
    M3Result result = ReadLebUnsigned(memory, &value, 7, io_bytes, i_end);
    *o_value = (u8)value;

    return result;
}

M3Result ReadLEB_i7(IM3Memory memory, i8* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!o_value) return m3Err_malformedData;

    i64 value;
    M3Result result = ReadLebSigned(memory, &value, 7, io_bytes, i_end);
    *o_value = (i8)value;

    return result;
}

M3Result ReadLEB_i32(IM3Memory memory, i32* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!o_value) return m3Err_malformedData;

    i64 value;
    M3Result result = ReadLebSigned(memory, &value, 32, io_bytes, i_end);
    *o_value = (i32)value;

    return result;
}

M3Result ReadLEB_i64(IM3Memory memory, i64* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!o_value) return m3Err_malformedData;

    i64 value;
    M3Result result = ReadLebSigned(memory, &value, 64, io_bytes, i_end);
    *o_value = (i64)value;

    return result;
}

M3Result Read_utf8(IM3Memory memory, cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_utf8) 
        return m3Err_malformedData;
        
    *o_utf8 = NULL;

    // Read the UTF-8 length
    u32 utf8Length;
    M3Result result = ReadLEB_u32(memory, &utf8Length, io_bytes, i_end);
    if (result != m3Err_none) return result;

    if (utf8Length > d_m3MaxSaneUtf8Length)
        return m3Err_missingUTF8;

    const u8* source_ptr;
    
    if (memory) {
        source_ptr = MEMACCESS(const u8*, memory, io_bytes);
        if (!source_ptr || source_ptr == ERROR_POINTER) 
            return m3Err_malformedData;
            
        // Check if we have enough space to read in segmented memory
        mos offset = get_offset_pointer(memory, (void*)source_ptr);
        mos end = get_offset_pointer(memory, (void*)i_end);
        if (offset + utf8Length > end) {
            __read_checkWasmUnderrun(offset, i_end);
    
            return m3Err_wasmUnderrun;
        }
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;

        source_ptr = (const u8*)*io_bytes;
        
        // Check if we have enough space to read in normal memory
        if (source_ptr + utf8Length > (const u8*)i_end) {
            __read_checkWasmUnderrun((CAST_PTR source_ptr + utf8Length), CAST_PTR i_end);
            return m3Err_wasmUnderrun;
        }
    }

    // I guess it's okay copying it to default malloc 
    // Anyway, it's possible to check if is related to M3Memory using IsValidMemory and use m3_Malloc instead
    // Allocate and copy the string
    char* utf8 = (char*)m3_Def_Malloc(utf8Length + 1); // name: "UTF8",
    if (!utf8) return m3Err_malformedData;

    m3_memcpy(memory, utf8, source_ptr, utf8Length);
    utf8[utf8Length] = 0;
    *o_utf8 = utf8;
    *io_bytes = source_ptr + utf8Length;

    return m3Err_none;
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

    M3BacktraceFrame * newFrame = m3_Def_AllocStruct(M3BacktraceFrame);

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
