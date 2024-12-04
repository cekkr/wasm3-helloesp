#ifndef m3_segmented_memory_h
#define m3_segmented_memory_h

#include <stddef.h>

#include "m3_core.h"
#include "m3_env.h"

#define M3Memory_MaxPages 1024

typedef struct MemorySegment {    
    void* data;           // Puntatore ai dati effettivi
    bool is_allocated;    // Flag per indicare se il segmento è stato allocato
    //size_t size;         // Dimensione del segmento
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

} M3Memory;

typedef M3Memory *          IM3Memory;

typedef struct M3MemoryPoint_t {  
    IM3Memory memory;
    size_t offset;
} M3MemoryPoint;

typedef M3MemoryPoint *          IM3MemoryPoint;

////////////////////////////////
IM3Memory m3_NewMemory();
IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem);

#endif
