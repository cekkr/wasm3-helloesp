#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "wasm3.h"
#include "m3_exception.h"

#include "esp_heap_caps.h"


#define WASM_SEGMENTED_MEM_ENABLE_HE_PAGES 1
#define m3_alloc_on_segment_data 0

#if WASM_SEGMENTED_MEM_ENABLE_HE_PAGES
#include "he_memory.h" 
#endif

#define WASM_ENABLE_SPI_MEM 0

#define WASM_INIT_SEGMENTS 64/4 // 64 KB // this solves the unrecognized M3Memory pointers
#define WASM_SEGMENT_SIZE 4096
#define WASM_CHUNK_SIZE 8  // Dimensione minima di un chunk di memoria

#define M3Memory_MaxPages 1024
#define M3Memory_PageSize 64*1024

#define INIT_FIRM 19942003
#define DUMMY_MEMORY_FIRM  6991 // Dummy M3Memory firm (to use when there is a placeholder memory)
#define M3PTR_FIRM 20190394

typedef struct MemoryChunk {
    size_t size;           // Total size including header
    bool is_free;          // Free flag
    struct MemoryChunk* next;
    struct MemoryChunk* prev;
    // New fields for multi-segment support
    uint16_t num_segments;     // Number of segments this chunk spans
    uint16_t start_segment;    // Starting segment index
    size_t* segment_sizes;     // Array of sizes in each segment
} MemoryChunk;

typedef struct ChunkInfo {
    MemoryChunk* chunk;
    u32 base_offset;
} ChunkInfo;

typedef struct MemorySegment {    
    int firm;

    void* data;           
    bool is_allocated;    
    size_t size;       
    u32 index;  
    MemoryChunk* first_chunk;  // Primo chunk nel segmento

    #if WASM_SEGMENTED_MEM_ENABLE_HE_PAGES
    segment_info_t* segment_page;
    #endif
} MemorySegment;

typedef struct M3Memory_t {  
    int firm;

    IM3Runtime runtime;
    u32 numPages;
    u32 maxPages;
    u32 pageSize;
    
    MemorySegment** segments;    
    size_t num_segments;        
    size_t total_size;
    size_t total_allocated_size;         
    size_t max_size;           
    size_t segment_size;
    size_t total_requested_size;
    
    // Cache per ottimizzare la ricerca di chunk liberi
    MemoryChunk** free_chunks;  // Array di puntatori a chunk liberi per size
    size_t num_free_buckets;    

    #if WASM_SEGMENTED_MEM_ENABLE_HE_PAGES
    paging_stats_t* paging;
    #endif
} M3Memory;

typedef M3Memory *          IM3Memory;

// Currently unused
typedef struct M3MemoryPoint_t {  
    u32 firm;
    IM3Memory memory;
    mos offset;
} M3MemoryPoint;

typedef M3MemoryPoint *          IM3MemoryPoint;

////////////////////////////////
IM3Memory m3_NewMemory();
IM3Memory m3_InitMemory(IM3Memory memory);
void FreeMemory(IM3Memory memory);
bool IsValidMemory(IM3Memory memory);

////////////////////////////////////////////////////////////////
IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem);
IM3MemoryPoint ValidateMemoryPoint(void* ptr);
////////////////////////////////////////////////////////////////

M3Result AddSegments(M3Memory* memory, size_t set_num_segments);
MemorySegment* InitSegment(M3Memory* memory, MemorySegment* seg, bool initData);
M3Result GrowMemory(M3Memory* memory, size_t additional_size);

////////////////////////////////////////////////////////////////

bool IsValidMemoryAccess(IM3Memory memory, mos offset, size_t size);
ptr get_segment_pointer(IM3Memory memory, mos offset);
ptr m3_ResolvePointer(M3Memory* memory, mos offset);
void* m3SegmentedMemAccess(IM3Memory mem, m3stack_t offset, size_t size);
mos get_offset_pointer(IM3Memory memory, void* ptr);

/// Regions 
ptr m3_malloc(M3Memory* memory, size_t size);
void m3_free(M3Memory* memory, ptr ptr);
ptr m3_realloc(M3Memory* memory, ptr ptr, size_t new_size);

M3Result m3_memset(M3Memory* memory, void* ptr, int value, size_t n);
M3Result m3_memcpy(M3Memory* memory, void* dest, const void* src, size_t n);

/// Garbage collection
void m3_collect_empty_segments(M3Memory* memory);

/// Memory chunks
ChunkInfo get_chunk_info(M3Memory* memory, void* ptr);

static u8 ERROR_POINTER[sizeof(u64)] __attribute__((aligned(8)));
