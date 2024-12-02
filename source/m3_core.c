//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#define M3_IMPLEMENT_ERROR_STRINGS
#include "m3_config.h"
#include "wasm3.h"

#include "m3_env.h"
#include "m3_core.h"

#include "m3_pointers.h"

#include "esp_debug_helpers.h"
#include "esp_heap_caps.h"

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

/*void *  m3_Malloc_Impl  (size_t i_size)
{
    u8 * ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    if (fixedHeapPtr >= fixedHeapEnd)
    {
        return NULL;
    }

    memset (ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    return ptr;
}

void  m3_Free_Impl  (void * i_ptr)
{
    // Handle the last chunk
    if (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast;
        fixedHeapLast = NULL;
    } else {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

void *  m3_Realloc_Impl  (void * i_ptr, size_t i_newSize, size_t i_oldSize)
{
    if (M3_UNLIKELY(i_newSize == i_oldSize)) return i_ptr;

    void * newPtr;

    // Handle the last chunk
    if (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        if (fixedHeapPtr >= fixedHeapEnd)
        {
            return NULL;
        }
        newPtr = i_ptr;
    } else {
        newPtr = m3_Malloc_Impl(i_newSize);
        if (!newPtr) {
            return NULL;
        }
        if (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    if (i_newSize > i_oldSize) {
        memset ((u8 *) newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    return newPtr;
}*/

#else

#define DEBUG_MEMORY 1

/*void* m3_Malloc_Impl(size_t i_size)
{
    if(DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Malloc_Impl of size %d", i_size);
    return heap_caps_calloc(1, i_size, MALLOC_CAP_8BIT);

    // Usa MALLOC_CAP_8BIT per memoria generale
    // Prima prova con PSRAM se disponibile
    void* ptr = heap_caps_malloc(i_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    
    if (!ptr) {
        ESP_LOGW("WASM3", "SPIRAM realloc failed, trying internal memory");
        ptr = heap_caps_malloc(i_size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    }

    if (ptr) {
        // Stessa pulizia incrementale
        const size_t block_size = 1024;
        uint8_t* p = (uint8_t*)ptr;
        size_t remaining = i_size;
        
        while (remaining > 0) {
            size_t to_clear = (remaining < block_size) ? remaining : block_size;
            memset(p, 0, to_clear);
            p += to_clear;
            remaining -= to_clear;
        }
    }
    return ptr;
}

void  m3_Free_Impl  (void * io_ptr)
{
    if(DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Free_Impl");
    heap_caps_free(io_ptr);
}*/

/*void* m3_Realloc_Impl_old(void* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    if(DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Realloc_Impl from %d to %d bytes", i_oldSize, i_newSize);
    
    if (M3_UNLIKELY(i_newSize == i_oldSize)) return i_ptr;

    // Usa gli stessi flag di allocazione del malloc
    void* newPtr = heap_caps_realloc(i_ptr, i_newSize, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

    if (M3_LIKELY(newPtr))
    {
        if (i_newSize > i_oldSize) {
            // Pulizia incrementale per blocchi grandi
            const size_t remainingSize = i_newSize - i_oldSize;
            const size_t block_size = 1024;
            uint8_t* p = (uint8_t*)newPtr + i_oldSize;
            size_t remaining = remainingSize;
            
            while (remaining > 0) {
                size_t to_clear = (remaining < block_size) ? remaining : block_size;
                memset(p, 0, to_clear);
                p += to_clear;
                remaining -= to_clear;
            }
        }
        return newPtr;
    }
    return NULL;
}*/

/*void* m3_Realloc_Impl(void* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    ESP_LOGI("WASM3", "Requesting realloc: old=%d new=%d", i_oldSize, i_newSize);
    
    // Dimensione massima di un segmento
    const size_t MAX_SEGMENT_SIZE = 32*1024; // 32KB
    
    // Se la richiesta è più piccola di un segmento, usa realloc normale
    if (i_newSize <= MAX_SEGMENT_SIZE) {
        return heap_caps_realloc(i_ptr, i_newSize, MALLOC_CAP_INTERNAL);
    }
    
    // Calcola quanti segmenti servono
    size_t num_segments = (i_newSize + MAX_SEGMENT_SIZE - 1) / MAX_SEGMENT_SIZE;
    
    // Alloca array di segmenti
    void** segments = heap_caps_malloc(sizeof(void*) * num_segments, MALLOC_CAP_INTERNAL);
    if (!segments) return NULL;
    
    bool success = true;
    for (size_t i = 0; i < num_segments && success; i++) {
        size_t segment_size = (i == num_segments-1) ? 
            (i_newSize - (i * MAX_SEGMENT_SIZE)) : MAX_SEGMENT_SIZE;
            
        segments[i] = heap_caps_malloc(segment_size, MALLOC_CAP_INTERNAL);
        success = (segments[i] != NULL);
    }
    
    if (!success) {
        // Cleanup in caso di fallimento
        for (size_t i = 0; i < num_segments; i++) {
            if (segments[i]) heap_caps_free(segments[i]);
        }
        heap_caps_free(segments);
        return NULL;
    }
    
    // Copia i dati vecchi se necessario
    if (i_ptr) {
        size_t remaining = i_oldSize;
        size_t offset = 0;
        
        while (remaining > 0) {
            size_t seg_idx = offset / MAX_SEGMENT_SIZE;
            size_t copy_size = (remaining > MAX_SEGMENT_SIZE) ? 
                MAX_SEGMENT_SIZE : remaining;
                
            memcpy(segments[seg_idx], 
                   (uint8_t*)i_ptr + offset, 
                   copy_size);
                   
            remaining -= copy_size;
            offset += copy_size;
        }
        
        heap_caps_free(i_ptr);
    }
    
    return segments[0];  // Ritorna il primo segmento
}*/

#endif

/*void *  m3_CopyMem  (const void * i_from, size_t i_size)
{
    if(DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_CopyMem");
    void * ptr = m3_Malloc("CopyMem", i_size);
    if (ptr) {
        memcpy (ptr, i_from, i_size);
    }
    return ptr;
}*/

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

    ESP_LOGI("WASM3", "Memoria totale libera: %d bytes\n", free_heap);
    ESP_LOGI("WASM3", "Blocco contiguo più grande: %d bytes\n", largest_block);

    // Per memoria interna (IRAM)
    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ESP_LOGI("WASM3", "Memoria interna libera: %d bytes\n", free_internal);

    // Per vedere la frammentazione
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    ESP_LOGI("WASM3", "Totale blocchi liberi: %d\n", info.total_free_bytes);
    ESP_LOGI("WASM3", "Minima memoria libera: %d bytes\n", info.minimum_free_bytes);  
}

static const int CHECK_MEMORY_AVAILABLE = 0;
//static const int WASM_ENABLE_SPI_MEM = 0;


// Allocatore di default che usa heap_caps
void* default_malloc(size_t size) {
    if(CHECK_MEMORY_AVAILABLE){
        print_memory_info();
    }

    void* ptr = WASM_ENABLE_SPI_MEM ? heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM) : NULL;
    if (ptr == NULL) {
        ptr = heap_caps_malloc(size, MALLOC_CAP_8BIT); // | MALLOC_CAP_INTERNAL
    }
    return ptr;
}

void default_free(void* ptr) {
    if(!safe_free(&ptr)) return;

    if (heap_caps_check_integrity_addr((intptr_t)ptr, false)) {
        heap_caps_free(ptr);
    }
    else {
        ESP_LOGW("WASM3", "Trying to free a memory outside heap limits");
        // esp_backtrace_print(100); #include "esp_debug_helpers.h"
    }
}

void* default_realloc(void* ptr, size_t new_size) {
    if(!safe_free(&ptr)){
        return default_malloc(new_size);
    }

    void* new_ptr = WASM_ENABLE_SPI_MEM ? heap_caps_realloc(ptr, new_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM) : NULL;
    if (new_ptr == NULL) {
        new_ptr = heap_caps_realloc(ptr, new_size, MALLOC_CAP_8BIT); //  | MALLOC_CAP_INTERNAL
    }
    return new_ptr;
}

void m3_SetMemoryAllocator(MemoryAllocator* allocator) {
    current_allocator = allocator ? allocator : &default_allocator;
}

// Funzione per allocare un segmento specifico
/*bool allocate_segment(M3Memory* memory, size_t segment_index) {
    if (segment_index >= memory->num_segments || 
        memory->segments[segment_index].is_allocated) {
        return false;
    }

    size_t segment_size = (segment_index == memory->num_segments - 1) ?
        (memory->total_size - (segment_index * memory->segment_size)) :
        memory->segment_size;

    void* data = current_allocator->malloc(segment_size);
    if (!data) return false;

    memory->segments[segment_index].data = data;
    memory->segments[segment_index].is_allocated = true;
    memory->segments[segment_index].size = segment_size;
    memset(data, 0, segment_size);

    return true;
}*/

void* m3_Malloc_Impl(size_t i_size) {
    if (DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Malloc_Impl of size %zu", i_size);

    M3Memory* memory = (M3Memory*)current_allocator->malloc(sizeof(M3Memory));
    if (!memory) {
        ESP_LOGE("WASM3", "Null M3Memory pointer");
        return NULL;
    }

    memory->segment_size = WASM_SEGMENT_SIZE;
    memory->total_size = i_size;
    memory->num_segments = (i_size + memory->segment_size - 1) / memory->segment_size;
    memory->point = 0;

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
        memory->segments[i].data = NULL;
        memory->segments[i].is_allocated = false;
        memory->segments[i].size = 0;
    }

    if (DEBUG_MEMORY) ESP_LOGI("WASM3", "Returning memory pointer");
    //esp_backtrace_print(100); 

    return memory;
}

void m3_Free_Impl(void* io_ptr, bool isMemory) {
    if (DEBUG_MEMORY) ESP_LOGI("WASM3", "Calling m3_Free_Impl");

    if (!safe_free(&io_ptr) || io_ptr == NULL) return;

    if (isMemory) {
        M3Memory* memory = (M3Memory*)io_ptr;
        
        if (memory->segments) {
            // Libera solo i segmenti effettivamente allocati
            for (size_t i = 0; i < memory->num_segments; i++) {
                if (memory->segments[i].is_allocated && memory->segments[i].data) {
                    if(safe_free(&memory->segments[i].data))
                        current_allocator->free(memory->segments[i].data);
                }
            }

            if(safe_free(&memory->segments))
                current_allocator->free(memory->segments);
        }
        
        if(!safe_free(&memory)){
            ESP_LOGI("WASM3", "m3_Free_Impl: not safe to free memory");
        }

        current_allocator->free(memory);
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
                MemorySegment* new_segments = current_allocator->realloc(
                    memory->segments,
                    new_num_segments * sizeof(MemorySegment)
                );

                if (!new_segments) return NULL;
                
                // Inizializza i nuovi segmenti se stiamo crescendo
                for (size_t i = memory->num_segments; i < new_num_segments; i++) {
                    new_segments[i].data = NULL;
                    new_segments[i].is_allocated = false;
                    new_segments[i].size = 0;
                }

                // Libera i segmenti in eccesso se stiamo riducendo
                for (size_t i = new_num_segments; i < memory->num_segments; i++) {
                    if (new_segments[i].is_allocated && new_segments[i].data) {
                        current_allocator->free(new_segments[i].data);
                    }
                }

                memory->segments = new_segments;
                memory->num_segments = new_num_segments;
                memory->total_size = i_newSize;
            }

            return i_ptr;
        }
        else {
            if(safe_free(&i_ptr)) current_allocator->free(i_ptr);
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
        if (src_memory->segments[i].is_allocated) {
            // Alloca e copia il segmento
            if (allocate_segment(dst_memory, i)) {
                size_t copy_size = M3_MIN(
                    src_memory->segments[i].size,
                    dst_memory->segments[i].size
                );
                memcpy(dst_memory->segments[i].data,
                       src_memory->segments[i].data,
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
                char * utf8 = (char *)m3_Malloc ("UTF8", utf8Length + 1);

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
        m3_Free (currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames = NULL;
    io_runtime->backtrace.lastFrame = NULL;
}
#endif // d_m3RecordBacktraces
