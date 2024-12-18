#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "wasm3.h"
#include "m3_exception.h"

#include "esp_heap_caps.h"

#define WASM_ENABLE_SPI_MEM 0

#define WASM_INIT_SEGMENTS 1
#define WASM_SEGMENT_SIZE 4096
#define WASM_CHUNK_SIZE 32  // Dimensione minima di un chunk di memoria
#define M3Memory_MaxPages 1024

#define INIT_FIRM 19942003

// Dummy M3Memory firm (to use when there is a placeholder memory)
#define DUMMY_MEMORY_FIRM  6991

typedef u32 mos; // memory offset

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

typedef struct MemorySegment {    
    int firm;

    void* data;           
    bool is_allocated;    
    size_t size;       
    u32 index;  
    MemoryChunk* first_chunk;  // Primo chunk nel segmento
} MemorySegment;

typedef struct M3Memory_t {  
    int firm;

    IM3Runtime runtime;
    u32 maxPages;
    
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
} M3Memory;

typedef M3Memory *          IM3Memory;

/* // Currently unused
typedef struct M3MemoryPoint_t {  
    IM3Memory memory;
    size_t offset;
} M3MemoryPoint;

typedef M3MemoryPoint *          IM3MemoryPoint;
*/

////////////////////////////////
IM3Memory m3_NewMemory();
IM3Memory m3_InitMemory(IM3Memory memory);
bool IsValidMemory(IM3Memory memory);
//IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem);

M3Result AddSegments(M3Memory* memory, size_t set_num_segments);
MemorySegment* InitSegment(M3Memory* memory, MemorySegment* seg, bool initData);
M3Result GrowMemory(M3Memory* memory, size_t additional_size);

////////////////////////////////////////////////////////////////

bool IsValidMemoryAccess(IM3Memory memory, mos offset, size_t size);
void* get_segment_pointer(IM3Memory memory, u32 offset);
void* resolve_pointer(M3Memory* memory, void* ptr);
void* m3SegmentedMemAccess(IM3Memory mem, void* offset, size_t size);
mos get_offset_pointer(IM3Memory memory, void* ptr);

/// Regions
void* m3_malloc(M3Memory* memory, size_t size);
void m3_free(M3Memory* memory, void* ptr);
void* m3_realloc(M3Memory* memory, void* ptr, size_t new_size);
void m3_memset(M3Memory* memory, void* ptr, int value, size_t n);

/// Garbage collection
void m3_collect_empty_segments(M3Memory* memory);

static u8 ERROR_POINTER[sizeof(u64)] __attribute__((aligned(8)));
