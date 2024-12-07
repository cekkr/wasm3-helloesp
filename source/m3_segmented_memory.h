#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "wasm3.h"

#define WASM_SEGMENT_SIZE 4096 // 4096 * x // btw move the definition elsewhere
#define WASM_PAGE_SIZE 65536 //todo: think about
#define WASM_ENABLE_SPI_MEM 0

#define M3Memory_MaxPages 1024

typedef enum {
    ADDRESS_INVALID = 0,
    ADDRESS_STACK,
    ADDRESS_LINEAR
} AddressType;

// Unified memory region structure
typedef struct M3MemoryRegion {
    u8* base;           // Base address of region
    size_t size;        // Current size
    size_t max_size;    // Maximum size
    size_t current_offset; // Current offset within region
} M3MemoryRegion;

// Memory segment with metadata
typedef struct MemorySegment {    
    void* data;           // Actual data pointer
    bool is_allocated;    // Allocation flag
    size_t stack_size;    // Current stack size in this segment
    size_t linear_size;   // Current linear memory size in this segment
} MemorySegment;

typedef struct M3Memory_t {  
    //M3MemoryHeader*       mallocated;    
    IM3Runtime              runtime;

    //u32                     initPages; // initPages or numPages?
    u32                     numPages;
    u32                     maxPages;
    //u32                     pageSize;

    // Segmentation
    MemorySegment* segments;    // Array di segmenti
    size_t num_segments;        // Current segments number
    size_t segment_size;        // Segment size
    size_t total_size;          // Current total size
    size_t max_size;            // Max size

    // Fragmentation
    M3MemoryRegion stack;      // Regione dello stack (cresce verso il basso)
    M3MemoryRegion linear;     // Regione della memoria lineare (cresce verso l'alto)
    //u8* stack_pointer;         // SP corrente // delegated to M3MemoryPoint

} M3Memory;

typedef M3Memory *          IM3Memory;

typedef struct M3MemoryPoint_t {  
    IM3Memory memory;
    size_t offset;
} M3MemoryPoint;

typedef M3MemoryPoint *          IM3MemoryPoint;

///
/// M3Memory fragmentation
///



////////////////////////////////
IM3Memory m3_NewMemory();
IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem);

void                        InitRuntime                 (IM3Runtime io_runtime, u32 i_stackSizeInBytes);
void                        Runtime_Release             (IM3Runtime io_runtime);
M3Result                    ResizeMemory                (IM3Runtime io_runtime, u32 i_numPages);

bool IsStackAddress(M3Memory* memory, u8* addr);
bool IsLinearAddress(M3Memory* memory, u8* addr);
u8* GetStackAddress(M3Memory* memory, size_t offset);
M3Result GrowStack(M3Memory* memory, size_t additional_size);

// Stack/linear addresses
// Studies: https://claude.ai/chat/699c9c02-0792-40c3-b08e-09b8e5df34c8
M3Result AddSegment(M3Memory* memory);
u8* GetEffectiveAddress(M3Memory* memory, size_t offset);
bool IsStackAddress(M3Memory* memory, u8* addr);
M3Result GrowStack(M3Memory* memory, size_t additional_size);

////////////////////////////////
bool allocate_segment(M3Memory* memory, size_t segment_index);
static inline void* GetMemorySegment(IM3Memory memory, u32 offset);
static inline i32 m3_LoadInt(IM3Memory memory, u32 offset);
static inline void m3_StoreInt(IM3Memory memory, u32 offset, i32 value);

