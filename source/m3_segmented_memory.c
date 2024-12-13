#include "m3_segmented_memory.h"
#include "esp_log.h"

#include "m3_pointers.h"

#define WASM_DEBUG_SEGMENTED_MEM_MAN 1

IM3Memory m3_NewMemory(){
    IM3Memory memory = m3_Int_AllocStruct (M3Memory);
    memory->segment_size = WASM_SEGMENT_SIZE;

    if(memory == NULL){
        ESP_LOGE("WASM3", "m3_NewMemory: Memory allocation failed");
        return NULL;
    }

    memory->segments = m3_Int_AllocArray(MemorySegment, WASM_INIT_SEGMENTS);    
    memory->max_size = 0; 
    memory->num_segments = 0;
    memory->total_size = 0;
    memory->segment_size = WASM_SEGMENT_SIZE;
    //memory->point = 0;

    // What are used for pages?
    //memory->numPages = 0;
    memory->maxPages = M3Memory_MaxPages;

    init_region_manager(&memory->region);

    return memory;
}

bool allocate_segment(M3Memory* memory, size_t segment_index) {
    if (!memory || segment_index >= memory->num_segments) {
        return false;
    }

    // Se il segmento è già allocato, restituisci true
    if (memory->segments[segment_index].is_allocated) {
        return true;
    }

    if (WASM_DEBUG_SEGMENTED_MEM_MAN) {
        ESP_LOGI("WASM3", "Allocating segment %zu of size %zu", 
                 segment_index, memory->segment_size);
    }

    // Prima prova ad allocare in SPIRAM se abilitata
    void* ptr = WASM_ENABLE_SPI_MEM ? 
                heap_caps_malloc(memory->segment_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM) : 
                NULL;

    // Se l'allocazione in SPIRAM fallisce o non è disponibile, prova la memoria interna
    if (!ptr) {
        ptr = heap_caps_malloc(memory->segment_size, 
                             MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        
        if (WASM_DEBUG_SEGMENTED_MEM_MAN && !ptr) {
            ESP_LOGE("WASM3", "Failed to allocate segment in internal memory");
        }
    }
    else if (WASM_DEBUG_SEGMENTED_MEM_MAN) {
        ESP_LOGI("WASM3", "Segment allocated in SPIRAM");
    }

    if (ptr) {
        // Inizializza il segmento con zeri
        memset(ptr, 0, memory->segment_size);
        
        memory->segments[segment_index].data = ptr;
        memory->segments[segment_index].is_allocated = true;
        
        if (WASM_DEBUG_SEGMENTED_MEM_MAN) {
            ESP_LOGI("WASM3", "Segment %zu successfully allocated", segment_index);
        }
        
        return true;
    }

    return false;
}

static void* GetMemorySegment(IM3Memory memory, u32 offset)
{
    size_t segment_index = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    MemorySegment segment = memory->segments[segment_index];
    return (u8*)segment.data + segment_offset;
}

bool IsValidMemoryAccess(IM3Memory memory, u64 offset, u32 size)
{
    return (offset + size) <= memory->total_size;
}

u8* GetSegmentPtr(IM3Memory memory, u64 offset, u32 size)
{
    size_t segment_index = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    
    if (M3_UNLIKELY(segment_index >= memory->num_segments ||
                    !memory->segments[segment_index].is_allocated ||
                    segment_offset + size > memory->segment_size))
    {
        return NULL;
    }
    
    return ((u8*)memory->segments[segment_index].data) + segment_offset;
}

M3Result GrowMemory(M3Memory* memory, size_t additional_size) {
    if (!memory) return m3Err_nullMemory;
    
    // Verifica limiti di memoria
    size_t new_total = memory->total_size + additional_size;
    if (new_total > memory->max_size) {
        return m3Err_memoryLimit;
    }
    
    // Calcola quanti nuovi segmenti servono
    size_t current_used_segments = (memory->total_size + memory->segment_size - 1) / memory->segment_size;
    size_t needed_segments = (new_total + memory->segment_size - 1) / memory->segment_size;
    size_t additional_segments = needed_segments - current_used_segments;
    
    // Alloca nuovi segmenti se necessario
    for (size_t i = 0; i < additional_segments; i++) {
        M3Result result = AddSegment(memory);
        if (result != NULL) {
            return result;
        }
    }
    
    memory->total_size = new_total;
    
    // Aggiorna il current_ptr se non ancora inizializzato
    if (!memory->current_ptr && memory->num_segments > 0) {
        memory->current_ptr = memory->segments[0].data;
    }
    
    return NULL;
}

/*IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem){
    IM3MemoryPoint point = m3_Int_AllocStruct (M3MemoryPoint);

    if(point == NULL){
        ESP_LOGE("WASM3", "m3_GetMemoryPoint: Memory allocation failed");
        return NULL;
    }

    point->memory = mem;
    point->offset = 0;
    return point;
}*/

// Funzione per aggiungere un nuovo segmento
M3Result AddSegment(M3Memory* memory) {
    size_t new_size = (memory->num_segments + 1) * sizeof(MemorySegment);
    MemorySegment* new_segments = m3_Int_Realloc("MemorySegment new_segments", memory->segments, new_size, 1);
    if (!new_segments) return m3Err_mallocFailed;
    
    memory->segments = new_segments;
    
    size_t new_idx = memory->num_segments;
    memory->segments[new_idx].data = m3_Int_Malloc("memory->segments[new_idx].data", memory->segment_size);
    if (!memory->segments[new_idx].data) return m3Err_mallocFailed;
    
    memory->segments[new_idx].is_allocated = true;
    memory->segments[new_idx].size = memory->segment_size;
    memory->num_segments++;
    memory->total_size += memory->segment_size;
    
    return NULL;
}

// Funzione per trovare segmento e offset di un indirizzo
static bool GetSegmentAndOffset(M3Memory* memory, u8* addr, size_t* seg_idx, size_t* offset) {
    for (size_t i = 0; i < memory->num_segments; i++) {
        if (!memory->segments[i].is_allocated) continue;
        
        u8* seg_start = memory->segments[i].data;
        u8* seg_end = seg_start + memory->segments[i].size;
        
        if (addr >= seg_start && addr < seg_end) {
            *seg_idx = i;
            *offset = addr - seg_start;
            return true;
        }
    }
    return false;
}

// Funzione per verificare validità di un indirizzo
bool IsValidAddress(M3Memory* memory, u8* addr) {
    size_t seg_idx, offset;
    return GetSegmentAndOffset(memory, addr, &seg_idx, &offset);
}

// Ottieni indirizzo effettivo con controllo dei limiti
u8* GetEffectiveAddress(M3Memory* memory, size_t offset) {
    size_t segment_idx = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    
    if (segment_idx >= memory->num_segments || 
        !memory->segments[segment_idx].is_allocated ||
        segment_offset >= memory->segments[segment_idx].size) {
        return NULL;
    }
    
    return (u8*)memory->segments[segment_idx].data + segment_offset;
}

///
/// Memory region
///

// Initialize region manager
void init_region_manager(RegionManager* mgr, size_t min_size) {
    mgr->head = NULL;
    mgr->min_region_size = min_size;
    mgr->total_regions = 0;
}

// Create new region for a segment
MemoryRegion* create_region(void* start, size_t size) {
    MemoryRegion* region = malloc(sizeof(MemoryRegion));
    if (!region) return NULL;
    
    region->start = start;
    region->size = size;
    region->is_free = true;
    region->next = NULL;
    region->prev = NULL;
    
    return region;
}

// Add region to manager
void add_region(RegionManager* mgr, MemoryRegion* region) {
    if (!mgr->head) {
        mgr->head = region;
    } else {
        // Insert sorted by address
        MemoryRegion* current = mgr->head;
        while (current->next && current->next->start < region->start) {
            current = current->next;
        }
        
        region->next = current->next;
        region->prev = current;
        if (current->next) {
            current->next->prev = region;
        }
        current->next = region;
    }
    mgr->total_regions++;
}

// Try to merge adjacent free regions
void coalesce_regions(RegionManager* mgr, MemoryRegion* region) {
    // Try to merge with next region
    if (region->next && region->next->is_free) {
        region->size += region->next->size;
        MemoryRegion* to_remove = region->next;
        region->next = to_remove->next;
        if (to_remove->next) {
            to_remove->next->prev = region;
        }
        free(to_remove);
        mgr->total_regions--;
    }
    
    // Try to merge with previous region
    if (region->prev && region->prev->is_free) {
        region->prev->size += region->size;
        region->prev->next = region->next;
        if (region->next) {
            region->next->prev = region->prev;
        }
        free(region);
        mgr->total_regions--;
    }
}

// Find a free region of sufficient size
MemoryRegion* find_free_region(RegionManager* mgr, size_t size) {
    MemoryRegion* current = mgr->head;
    while (current) {
        if (current->is_free && current->size >= size) {
            // Split region if remaining size is worth it
            if (current->size >= size + mgr->min_region_size) {
                MemoryRegion* new_region = create_region(
                    (char*)current->start + size,
                    current->size - size
                );
                current->size = size;
                
                new_region->next = current->next;
                new_region->prev = current;
                if (current->next) {
                    current->next->prev = new_region;
                }
                current->next = new_region;
                mgr->total_regions++;
            }
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Updated malloc to use regions
void* m3_malloc(M3Memory* memory, size_t size) {
    if (!memory || size == 0) {
        return NULL;
    }

    // Round up size to alignment
    size_t aligned_size = ((size + 7) & ~7);  // 8-byte alignment
    
    // Try to find existing free region
    MemoryRegion* region = find_free_region(&memory->region_mgr, aligned_size);
    
    if (!region) {
        // Need to allocate new segment
        size_t segment_size = ((aligned_size + memory->segment_size - 1) 
                              / memory->segment_size) * memory->segment_size;
        
        int segment_index = create_new_segment(memory, segment_size);
        if (segment_index == -1) {
            return NULL;
        }
        
        // Create and add region for new segment
        region = create_region(memory->segments[segment_index].data, segment_size);
        if (!region) {
            return NULL;
        }
        add_region(&memory->region_mgr, region);
    }
    
    region->is_free = false;
    return region->start;
}

// Updated free to use regions
void m3_free(M3Memory* memory, void* ptr) {
    if (!memory || !ptr) {
        return;
    }

    // Find region containing this pointer
    MemoryRegion* current = memory->region_mgr.head;
    while (current) {
        if (current->start == ptr) {
            current->is_free = true;
            coalesce_regions(&memory->region_mgr, current);
            return;
        }
        current = current->next;
    }
}

// Updated realloc to use regions
void* m3_realloc(M3Memory* memory, void* ptr, size_t new_size) {
    if (!memory) {
        return NULL;
    }
    
    if (!ptr) {
        return m3_malloc(memory, new_size);
    }
    
    if (new_size == 0) {
        m3_free(memory, ptr);
        return NULL;
    }

    // Round up new size
    size_t aligned_size = ((new_size + 7) & ~7);  // 8-byte alignment

    // Find current region
    MemoryRegion* current = memory->region_mgr.head;
    while (current && current->start != ptr) {
        current = current->next;
    }
    
    if (!current) {
        return NULL;  // Invalid pointer
    }

    // If current region is big enough, just return same pointer
    if (current->size >= aligned_size) {
        // Could split if remaining size is large enough
        if (current->size >= aligned_size + memory->region_mgr.min_region_size) {
            MemoryRegion* new_region = create_region(
                (char*)current->start + aligned_size,
                current->size - aligned_size
            );
            current->size = aligned_size;
            
            new_region->next = current->next;
            new_region->prev = current;
            if (current->next) {
                current->next->prev = new_region;
            }
            current->next = new_region;
            memory->region_mgr.total_regions++;
        }
        return ptr;
    }

    // Need to allocate new region
    void* new_ptr = m3_malloc(memory, new_size);
    if (!new_ptr) {
        return NULL;
    }

    // Copy old data and free old region
    memcpy(new_ptr, ptr, current->size);
    m3_free(memory, ptr);

    return new_ptr;
}