#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "wasm3.h"
#include "m3_exception.h"

#include "esp_heap_caps.h"

#define WASM_SEGMENT_SIZE 4096 // 4096 * x // btw move the definition elsewhere
#define WASM_INIT_SEGMENTS 16 // useless, due to the use of reallocation
//#define WASM_PAGE_SIZE 65536 // deprecated
#define WASM_ENABLE_SPI_MEM 0
#define WASM_M3MEMORY_REGION_MIN_SIZE 32

#define M3Memory_MaxPages 1024

typedef u32 iptr;

typedef struct MemorySegment {    
    void* data;           // Actual data pointer
    bool is_allocated;    // Allocation flag
    size_t size;         // Segment size
} MemorySegment;

typedef struct MemoryRegion {
    void* start;          // Start address of region
    size_t size;         // Size of region
    bool is_free;        // Whether region is free
    struct MemoryRegion* next;  // Next region in list
    struct MemoryRegion* prev;  // Previous region in list
} MemoryRegion;

typedef struct RegionManager {
    MemoryRegion* head;       // Head of region list
    size_t min_region_size;   // Minimum region size for splitting
    size_t total_regions;     // Total number of regions
} RegionManager;

typedef struct M3Memory_t {  
    IM3Runtime runtime;

    // Memory info
    u32 maxPages;
    u32 pageSize;

    // Segmentation
    MemorySegment* segments;    // Array of segments
    size_t num_segments;        // Current number of segments
    size_t total_size;         // Current total size
    size_t max_size;           // Maximum allowed size
    size_t segment_size;

    // Region management
    RegionManager region_mgr;   // Region manager

    // Current memory tracking
    u8* current_ptr;           // Current position in memory
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
//IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem);

M3Result AddSegment(M3Memory* memory);
u8* GetEffectiveAddress(M3Memory* memory, size_t offset);
M3Result GrowMemory(M3Memory* memory, size_t additional_size);

////////////////////////////////
bool allocate_segment(M3Memory* memory, size_t segment_index);
static void* GetMemorySegment(IM3Memory memory, u32 offset);
////////////////////////////////////////////////////////////////

bool IsValidMemoryAccess(IM3Memory memory, u64 offset, u32 size);
u8* GetSegmentPtr(IM3Memory memory, u64 offset, u32 size);

u8* m3SegmentedMemAccess(IM3Memory mem, iptr offset, size_t size);

/// Regions
void* m3_malloc(M3Memory* memory, size_t size);
void m3_free(M3Memory* memory, void* ptr);
void* m3_realloc(M3Memory* memory, void* ptr, size_t new_size);

void init_region_manager(RegionManager* mgr, size_t min_size);