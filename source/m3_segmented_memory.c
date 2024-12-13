#include "m3_segmented_memory.h"
#include "esp_log.h"

#include "m3_pointers.h"

#define WASM_DEBUG_SEGMENTED_MEM_MAN 1

IM3Memory m3_NewMemory(){
    IM3Memory memory = m3_Def_AllocStruct (M3Memory);

    if(memory == NULL){
        ESP_LOGE("WASM3", "m3_NewMemory: Memory allocation failed");
        return NULL;
    }

    memory->segments = m3_Def_AllocArray(MemorySegment, WASM_INIT_SEGMENTS);    
    memory->max_size = 0; 
    memory->num_segments = 0;
    memory->total_size = 0;
    memory->segment_size = WASM_SEGMENT_SIZE;
    //memory->point = 0;

    // What are used for pages?
    //memory->numPages = 0;
    memory->maxPages = M3Memory_MaxPages;

    init_region_manager(&memory->region_mgr, WASM_M3MEMORY_REGION_MIN_SIZE);

    return memory;
}

IM3Memory m3_InitMemory(IM3Memory memory){

    if(memory == NULL){
        ESP_LOGE("WASM3", "m3_InitMemory: Memory is null");
        return NULL;
    }

    memory->segments = m3_Def_AllocArray(MemorySegment, WASM_INIT_SEGMENTS);    
    memory->max_size = 0; 
    memory->num_segments = 0;
    memory->total_size = 0;
    memory->segment_size = WASM_SEGMENT_SIZE;
    //memory->point = 0;

    // What are used for pages?
    //memory->numPages = 0;
    memory->maxPages = M3Memory_MaxPages;

    init_region_manager(&memory->region_mgr, WASM_M3MEMORY_REGION_MIN_SIZE);

    return memory;
}

bool allocate_segment(M3Memory* memory, size_t segment_index) {
    if (!memory || segment_index >= memory->num_segments) {
        return false;
    }

    // Se il segmento è già allocato, restituisci true
    if (memory->segments[segment_index]->is_allocated) {
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
        
        memory->segments[segment_index]->data = ptr;
        memory->segments[segment_index]->is_allocated = true;
        
        if (WASM_DEBUG_SEGMENTED_MEM_MAN) {
            ESP_LOGI("WASM3", "Segment %zu successfully allocated", segment_index);
        }
        
        return true;
    }

    return false;
}

void* GetMemorySegment(IM3Memory memory, u32 offset)
{
    size_t segment_index = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    MemorySegment* segment = memory->segments[segment_index];
    return (u8*)segment->data + segment_offset;
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
                    !memory->segments[segment_index]->is_allocated ||
                    segment_offset + size > memory->segment_size))
    {
        return NULL;
    }
    
    return ((u8*)memory->segments[segment_index]->data) + segment_offset;
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
        memory->current_ptr = memory->segments[0]->data;
    }
    
    return NULL;
}

/*IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem){
    IM3MemoryPoint point = m3_Def_AllocStruct (M3MemoryPoint);

    if(point == NULL){
        ESP_LOGE("WASM3", "m3_GetMemoryPoint: Memory allocation failed");
        return NULL;
    }

    point->memory = mem;
    point->offset = 0;
    return point;
}*/

// Funzione per aggiungere un nuovo segmento
const bool WASM_DEBUG_ADD_SEGMENT = true;
M3Result AddSegment(M3Memory* memory) {
    size_t new_idx = memory->num_segments++;
    size_t new_size = (memory->num_segments) * sizeof(MemorySegment*);
    
    // Riallocare l'array di puntatori
    MemorySegment** new_segments = m3_Def_Realloc(memory->segments, new_size);
    if (!new_segments) {
        memory->num_segments--;
        return m3Err_mallocFailed;
    }
    memory->segments = new_segments;
    
    // Allocare la nuova struttura MemorySegment
    memory->segments[new_idx] = m3_Def_Malloc(sizeof(MemorySegment));
    if (!memory->segments[new_idx]) {
        memory->num_segments--;
        return m3Err_mallocFailed;
    }
    
    if(WASM_DEBUG_ADD_SEGMENT){
         ESP_LOGI("WASM3", "AddSegment: memory->segments[%d]=%p", new_idx, memory->segments[new_idx]);
    }

        // Verifica che il puntatore sia valido prima di usarlo
    if (!ultra_safe_ptr_valid(memory->segments[new_idx])) {
        ESP_LOGE("WASM3", "AddSegment: Invalid pointer at memory->segments[%d]", new_idx);
        return m3Err_mallocFailed;
    }
    
    // Verifica che possiamo effettivamente accedere alla struttura
    MemorySegment* seg = memory->segments[new_idx];
    if(WASM_DEBUG_ADD_SEGMENT) ESP_LOGI("WASM3", "AddSegment: Can access segment structure");

    // Allocare i dati del segmento
    seg->data = m3_Def_Malloc(memory->segment_size);
    if (!seg->data) {
        m3_Def_Free(seg);
        memory->num_segments--;
        return m3Err_mallocFailed;
    }
    
    seg->is_allocated = true;
    seg->size = memory->segment_size;
    memory->total_size += memory->segment_size;
    
    return NULL;
}

// Funzione per trovare segmento e offset di un indirizzo
bool GetSegmentAndOffset(M3Memory* memory, u8* addr, size_t* seg_idx, size_t* offset) {
    for (size_t i = 0; i < memory->num_segments; i++) {
        if (!memory->segments[i]->is_allocated) continue;
        
        u8* seg_start = memory->segments[i]->data;
        u8* seg_end = seg_start + memory->segments[i]->size;
        
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
        !memory->segments[segment_idx]->is_allocated ||
        segment_offset >= memory->segments[segment_idx]->size) {
        return NULL;
    }
    
    return (u8*)memory->segments[segment_idx]->data + segment_offset;
}

const bool WASM_DEBUG_SEGMENTED_MEM_ACCESS = true;

u8* m3SegmentedMemAccess(IM3Memory mem, iptr offset, size_t size) 
{
    //return (u8*)offset;

    if(mem == NULL){
        ESP_LOGE("WASM3", "m3SegmentedMemAccess called with null memory pointer");     
        backtrace();
        return NULL;
    }

    if(WASM_DEBUG_SEGMENTED_MEM_ACCESS){ 
        ESP_LOGI("WASM3", "m3SegmentedMemAccess call");         
        ESP_LOGI("WASM3", "m3SegmentedMemAccess: mem = %p", (void*)mem);
        ESP_LOGI("WASM3", "m3SegmentedMemAccess: well... I'm going to crash!");    
    }

    // Verifica che l'accesso sia nei limiti della memoria totale
    if (mem->total_size > 0 && offset + size > mem->total_size) 
        return NULL;

    size_t segment_index = offset / mem->segment_size;
    size_t segment_offset = offset % mem->segment_size;
    
    // Verifica se stiamo accedendo attraverso più segmenti
    size_t end_segment = (offset + size - 1) / mem->segment_size;
    
    // Alloca tutti i segmenti necessari se non sono già allocati
    for (size_t i = segment_index; i <= end_segment; i++) {
        if (!mem->segments[i]->is_allocated) {
            if (!allocate_segment(mem, i)) {
                ESP_LOGE("WASM3", "Failed to allocate segment %zu on access", i);
                return NULL;
            }
            if(WASM_DEBUG_SEGMENTED_MEM_ACCESS) ESP_LOGI("WASM3", "Lazy allocated segment %zu on access", i);
        }
    }
    
    // Ora possiamo essere sicuri che il segmento è allocato
    return ((u8*)mem->segments[segment_index]->data) + segment_offset;
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

#define MIN(x, y) ((x) < (y) ? (x) : (y))

// Custom memcpy for potentially cross-segment regions
void m3_memcpy(M3Memory* memory, void* dest, const void* src, size_t n) {
    u8* d = (u8*)dest;
    const u8* s = (const u8*)src;
    
    while (n > 0) {
        // Calculate current segment boundaries
        size_t src_offset = (size_t)s - (size_t)memory->segments[0]->data;
        size_t dest_offset = (size_t)d - (size_t)memory->segments[0]->data;
        
        // Get source and destination segments
        u8* src_ptr = m3SegmentedMemAccess(memory, src_offset, 1);
        u8* dest_ptr = m3SegmentedMemAccess(memory, dest_offset, 1);
        
        if (!src_ptr || !dest_ptr) {
            // Handle error - invalid memory access
            return;
        }
        
        // Calculate how many bytes we can copy within current segments
        size_t src_segment_end = (src_offset / memory->segment_size + 1) * memory->segment_size;
        size_t dest_segment_end = (dest_offset / memory->segment_size + 1) * memory->segment_size;
        size_t bytes_to_copy = n;
        
        // Limit copy to smallest segment boundary
        bytes_to_copy = MIN(bytes_to_copy, src_segment_end - src_offset);
        bytes_to_copy = MIN(bytes_to_copy, dest_segment_end - dest_offset);
        
        // Perform the copy
        memcpy(dest_ptr, src_ptr, bytes_to_copy);
        
        // Update pointers and remaining size
        d += bytes_to_copy;
        s += bytes_to_copy;
        n -= bytes_to_copy;
    }
}

// Helper to check if a region spans multiple segments
bool is_cross_segment_region(M3Memory* memory, void* start, size_t size) {
    size_t start_offset = (size_t)start - (size_t)memory->segments[0]->data;
    size_t end_offset = start_offset + size - 1;
    return (start_offset / memory->segment_size) != (end_offset / memory->segment_size);
}

// Updated region allocation to handle non-resident segments
MemoryRegion* allocate_region(M3Memory* memory, size_t size) {
    size_t aligned_size = ((size + 7) & ~7);  // 8-byte alignment
    size_t required_segments = (aligned_size + memory->segment_size - 1) / memory->segment_size;
    
    // Find continuous range of segments
    size_t start_segment = 0;
    size_t continuous_segments = 0;
    
    for (size_t i = 0; i < memory->num_segments; i++) {
        if (!memory->segments[i]->is_allocated) {
            if (continuous_segments == 0) {
                start_segment = i;
            }
            continuous_segments++;
            
            if (continuous_segments >= required_segments) {
                // Found enough continuous segments
                for (size_t j = 0; j < required_segments; j++) {
                    if (!allocate_segment(memory, start_segment + j)) {
                        // Rollback previous allocations on failure
                        while (j > 0) {
                            j--;
                            memory->segments[start_segment + j]->is_allocated = false;
                        }
                        return NULL;
                    }
                }
                
                // Create new region
                void* region_start = GetMemorySegment(memory, start_segment * memory->segment_size);
                MemoryRegion* region = create_region(region_start, aligned_size);
                return region;
            }
        } else {
            continuous_segments = 0;
        }
    }
    
    // Need to grow memory
    size_t additional_segments = required_segments - continuous_segments;
    M3Result result = GrowMemory(memory, additional_segments * memory->segment_size);
    if (result != m3Err_none) {
        return NULL;
    }
    
    // Retry allocation after growth
    return allocate_region(memory, size);
}

// Split a region into two parts
void split_region(RegionManager* mgr, MemoryRegion* region, size_t size) {
    if (!mgr || !region || size >= region->size) {
        return;
    }
    
    // Create new region for the remaining space
    MemoryRegion* new_region = create_region(
        (char*)region->start + size,
        region->size - size
    );
    
    if (!new_region) {
        return;
    }
    
    // Set properties for new region
    new_region->is_free = true;
    
    // Insert new region into the linked list
    new_region->next = region->next;
    new_region->prev = region;
    if (region->next) {
        region->next->prev = new_region;
    }
    region->next = new_region;
    
    // Update size of original region
    region->size = size;
    
    // Update region count
    mgr->total_regions++;
}

const bool WASM_DEBUG_SEGMENTED_MEMORY = true;

// Updated malloc to handle segment loading
void* m3_malloc(M3Memory* memory, size_t size) {
    if(WASM_DEBUG_SEGMENTED_MEMORY) {
        ESP_LOGI("WASM3", "m3_malloc called");
        ESP_LOGI("WASM3", "m3_malloc: memory->segment_size = %zu", memory->segment_size);
    }

    if (!memory || size == 0) {
        ESP_LOGE("WASM3", "m3_malloc: memory NULL or size = %d", size);
        return NULL;
    }
    
    // Try to find existing free region
    MemoryRegion* region = find_free_region(&memory->region_mgr, size);
    if (region) {
        // Ensure all segments in region are loaded
        size_t start_offset = (size_t)region->start - (size_t)memory->segments[0]->data;
        for (size_t offset = 0; offset < region->size; offset += memory->segment_size) {
            if (!m3SegmentedMemAccess(memory, start_offset + offset, 1)) {
                return NULL;
            }
        }
        region->is_free = false;
        return region->start;
    }
    
    // Allocate new region
    region = allocate_region(memory, size);
    if (!region) {
        return NULL;
    }
    
    add_region(&memory->region_mgr, region);
    region->is_free = false;
    return region->start;
}

// Updated realloc to handle cross-segment copies
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
    
    // Find current region
    MemoryRegion* current = memory->region_mgr.head;
    while (current && current->start != ptr) {
        current = current->next;
    }
    
    if (!current) {
        return NULL;  // Invalid pointer
    }
    
    size_t aligned_size = ((new_size + 7) & ~7);
    
    // If current region is big enough, just return same pointer
    if (current->size >= aligned_size) {
        // Could split if remaining size is large enough
        if (current->size >= aligned_size + memory->region_mgr.min_region_size) {
            split_region(&memory->region_mgr, current, aligned_size);
        }
        return ptr;
    }
    
    // Need to allocate new region
    void* new_ptr = m3_malloc(memory, new_size);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy data using cross-segment aware memcpy
    m3_memcpy(memory, new_ptr, ptr, MIN(current->size, new_size));
    m3_free(memory, ptr);
    
    return new_ptr;
}

// Free remains mostly the same but needs to handle segment deallocation
void m3_free(M3Memory* memory, void* ptr) {
    if (!memory || !ptr) {
        return;
    }
    
    MemoryRegion* current = memory->region_mgr.head;
    while (current) {
        if (current->start == ptr) {
            // Mark segments as unallocated if no other regions use them
            size_t start_offset = (size_t)current->start - (size_t)memory->segments[0]->data;
            size_t end_offset = start_offset + current->size;
            
            for (size_t offset = start_offset; offset < end_offset; offset += memory->segment_size) {
                size_t segment_index = offset / memory->segment_size;
                bool segment_in_use = false;
                
                // Check if any other region uses this segment
                MemoryRegion* other = memory->region_mgr.head;
                while (other) {
                    if (other != current && !other->is_free) {
                        size_t other_start = (size_t)other->start - (size_t)memory->segments[0]->data;
                        size_t other_end = other_start + other->size;
                        
                        if (offset >= other_start && offset < other_end) {
                            segment_in_use = true;
                            break;
                        }
                    }
                    other = other->next;
                }
                
                if (!segment_in_use) {
                    memory->segments[segment_index]->is_allocated = false;
                }
            }
            
            current->is_free = true;
            coalesce_regions(&memory->region_mgr, current);
            return;
        }
        current = current->next;
    }
}