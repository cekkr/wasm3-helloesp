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

// Strutture dati migliorate
typedef struct MemoryChunk {
    size_t size;           // Dimensione totale del chunk incluso header
    bool is_free;          // Flag per chunk libero
    struct MemoryChunk* next;
    struct MemoryChunk* prev;
} MemoryChunk;

typedef struct MemorySegment {    
    void* data;           
    bool is_allocated;    
    size_t size;         
    MemoryChunk* first_chunk;  // Primo chunk nel segmento
} MemorySegment;

typedef struct M3Memory_t {  
    IM3Runtime runtime;
    u32 maxPages;
    
    MemorySegment** segments;    
    size_t num_segments;        
    size_t total_size;         
    size_t max_size;           
    size_t segment_size;
    
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
//IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem);

M3Result AddSegment(M3Memory* memory, size_t set_num_segments);
M3Result GrowMemory(M3Memory* memory, size_t additional_size);

////////////////////////////////
bool allocate_segment_data(M3Memory* memory, size_t segment_index);
static void* GetMemorySegment(IM3Memory memory, u32 offset);
////////////////////////////////////////////////////////////////

bool IsValidMemoryAccess(IM3Memory memory, u64 offset, u32 size);
void* resolve_pointer(M3Memory* memory, void* ptr);
void* m3SegmentedMemAccess(IM3Memory mem, void* offset, size_t size);
void* m3SegmentedMemAccess_2(IM3Memory memory, u32 offset, size_t size);

/// Regions
void* m3_malloc(M3Memory* memory, size_t size);
void m3_free(M3Memory* memory, void* ptr);
void* m3_realloc(M3Memory* memory, void* ptr, size_t new_size);
