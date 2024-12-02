#ifndef m3_segmented_memory_h
#define m3_segmented_memory_h

#include <stddef.h>

#include "m3_core.h"

typedef struct MemorySegment {    
    void* data;           // Puntatore ai dati effettivi
    bool is_allocated;    // Flag per indicare se il segmento è stato allocato
    //size_t size;         // Dimensione del segmento
} MemorySegment;

typedef struct M3Memory_t {
    u32                     numPages;
    u32                     maxPages;
    u32                     pageSize;

    // Segmentation
    MemorySegment* segments;    // Array di segmenti
    size_t num_segments;        // Numero totale di segmenti
    size_t segment_size;        // Dimensione di ogni segmento
    size_t total_size;          // Dimensione totale richiesta

    // Pointer
    size_t point;
} M3Memory;

typedef M3Memory *          IM3Memory;


#endif
