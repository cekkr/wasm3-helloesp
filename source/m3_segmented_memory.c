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


#if M3Memory_Simplified

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

// Funzione per crescere la memoria
/*M3Result GrowMemory(M3Memory* memory, size_t additional_size) { // older implementation
    size_t new_total_size = memory->total_size + additional_size;
    
    if (memory->max_size > 0 && new_total_size > memory->max_size) {
        return m3Err_memoryLimit;
    }
    
    // Calcola quanti nuovi segmenti servono
    size_t needed_segments = (additional_size + memory->segment_size - 1) / memory->segment_size;
    
    for (size_t i = 0; i < needed_segments; i++) {
        M3Result result = AddSegment(memory);
        if (result != NULL) return result;
    }
    
    return NULL;
}*/

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

// Inizializzazione memoria
M3Result InitMemory(M3Memory* memory, size_t initial_size) {
    memory->segment_size = WASM_SEGMENT_SIZE;
    
    size_t num_segments = (initial_size + memory->segment_size - 1) / memory->segment_size;
    
    memory->segments = m3_Int_Malloc("memory->segments", num_segments * sizeof(MemorySegment));
    if (!memory->segments) return m3Err_mallocFailed;
    
    memory->num_segments = 0;  // Sarà incrementato da AddSegment
    memory->total_size = 0;    // Sarà incrementato da AddSegment
    memory->current_ptr = NULL;
    
    // Alloca tutti i segmenti necessari
    for (size_t i = 0; i < num_segments; i++) {
        M3Result result = AddSegment(memory);
        if (result != NULL) {
            // Cleanup in caso di errore
            for (size_t j = 0; j < memory->num_segments; j++) {
                if (memory->segments[j].is_allocated) {
                    m3_Int_Free(memory->segments[j].data);
                }
            }
            m3_Int_Free(memory->segments);
            return result;
        }
    }
    
    memory->current_ptr = memory->segments[0].data;
    return NULL;
}

#else

///
/// Memory fragmentation
///

// Funzione per verificare se un indirizzo appartiene allo stack
bool IsStackAddress(M3Memory* memory, u8* addr) {
    return (addr <= memory->stack.base && 
            addr >= (memory->stack.base - memory->stack.size));
}

// Funzione per verificare se un indirizzo appartiene alla memoria lineare
bool IsLinearAddress(M3Memory* memory, u8* addr) {
    return (addr >= memory->linear.base && 
            addr < (memory->linear.base + memory->linear.size));
}

// Funzione per convertire offset stack in indirizzo assoluto
u8* GetStackAddress(M3Memory* memory, size_t offset) {
    return memory->stack.base - offset;
}

///
/// Stack/linear differation
///

// Funzione per aggiungere un nuovo segmento
M3Result AddSegment(M3Memory* memory) {
    size_t new_size = (memory->num_segments + 1) * sizeof(MemorySegment);
    MemorySegment* new_segments = m3_Int_Realloc("MemorySegment new_segments", memory->segments, new_size, 1); // 1 is old size
    if (!new_segments) return m3Err_mallocFailed;
    
    memory->segments = new_segments;
    
    // Inizializza il nuovo segmento
    size_t new_idx = memory->num_segments;
    memory->segments[new_idx].data = m3_Int_Malloc("memory->segments[new_idx].data", memory->segment_size);
    if (!memory->segments[new_idx].data) return m3Err_mallocFailed;
    
    memory->segments[new_idx].is_allocated = true;
    memory->num_segments++;
    memory->total_size += memory->segment_size;
    
    return NULL; // aka success
}

// Funzione per verificare se un indirizzo appartiene allo stack
bool IsStackAddress(M3Memory* memory, u8* addr) {
    for (size_t i = 0; i < memory->num_segments; i++) {
        u8* seg_start = memory->segments[i].data;
        u8* seg_end = seg_start + memory->segment_size;
        
        if (addr >= seg_start && addr < seg_end) {
            size_t offset = addr - seg_start;
            size_t total_offset = i * memory->segment_size + offset;
            
            // Verifica se l'offset totale è nella regione stack
            return total_offset >= memory->total_size - memory->stack.size;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////

///
/// New implementation
///


// Get segment and offset for a given address
static bool GetSegmentAndOffset(M3Memory* memory, u8* addr, size_t* seg_idx, size_t* offset) {
    for (size_t i = 0; i < memory->num_segments; i++) {
        if (!memory->segments[i].is_allocated) continue;
        
        u8* seg_start = memory->segments[i].data;
        u8* seg_end = seg_start + memory->segment_size;
        
        if (addr >= seg_start && addr < seg_end) {
            *seg_idx = i;
            *offset = addr - seg_start;
            return true;
        }
    }
    return false;
}

// Unified function to check address type
AddressType GetAddressType(M3Memory* memory, u8* addr) {
    size_t seg_idx, offset;
    
    if (!GetSegmentAndOffset(memory, addr, &seg_idx, &offset)) {
        return ADDRESS_INVALID;
    }
    
    MemorySegment* segment = &memory->segments[seg_idx];
    size_t total_offset = seg_idx * memory->segment_size + offset;
    
    if (total_offset >= memory->total_size - memory->stack.size) {
        return ADDRESS_STACK;
    }
    
    if (total_offset < memory->linear.size) {
        return ADDRESS_LINEAR;
    }
    
    return ADDRESS_INVALID;
}

// Unified stack growth function
M3Result GrowStack(M3Memory* memory, size_t additional_size) {
    size_t new_stack_size = memory->stack.size + additional_size;
    
    // Check if we need a new segment
    if (memory->linear.size + new_stack_size > memory->total_size) {
        // Try to allocate new segment first
        M3Result result = AddSegment(memory);
        if (result != NULL) {
            return result;
        }
        
        // Update stack base to end of new segment
        size_t last_seg = memory->num_segments - 1;
        memory->stack.base = (u8*)memory->segments[last_seg].data + memory->segment_size;
        
        // Update segment metadata
        memory->segments[last_seg].stack_size = additional_size;
    } else {
        // Find segment containing stack top
        size_t seg_idx = (memory->total_size - memory->stack.size) / memory->segment_size;
        memory->segments[seg_idx].stack_size += additional_size;
    }
    
    memory->stack.size = new_stack_size;
    return NULL;
}

// Get effective address with bounds checking
u8* GetEffectiveAddress(M3Memory* memory, size_t offset) {
    size_t segment_idx = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    
    if (segment_idx >= memory->num_segments || 
        !memory->segments[segment_idx].is_allocated) {
        return NULL;
    }
    
    // Check if address is in valid region
    if (offset >= memory->linear.size && 
        offset < memory->total_size - memory->stack.size) {
        return NULL;
    }
    
    return (u8*)memory->segments[segment_idx].data + segment_offset;
}

// Memory initialization with proper segmentation
M3Result InitMemory(M3Memory* memory, size_t initial_stack, size_t initial_linear) {
    memory->segment_size = WASM_SEGMENT_SIZE;
    
    // Calculate required segments
    size_t total_size = initial_stack + initial_linear;
    size_t num_segments = (total_size + memory->segment_size - 1) / memory->segment_size;
    
    memory->segments = m3_Int_Malloc("memory->segments", num_segments * sizeof(MemorySegment));
    if (!memory->segments) return m3Err_mallocFailed;
    
    // Initialize segments
    for (size_t i = 0; i < num_segments; i++) {
        memory->segments[i].data = NULL;
        memory->segments[i].is_allocated = false;
        memory->segments[i].stack_size = 0;
        memory->segments[i].linear_size = 0;
    }
    
    // Allocate first segment
    if (!allocate_segment(memory, 0)) {
        m3_Int_Free(memory->segments);
        return m3Err_mallocFailed;
    }
    
    // Setup memory regions
    memory->linear.base = memory->segments[0].data;
    memory->linear.size = initial_linear;
    memory->linear.current_offset = 0;
    
    memory->stack.base = (u8*)memory->segments[num_segments-1].data + memory->segment_size;
    memory->stack.size = initial_stack;
    memory->stack.current_offset = 0;
    
    memory->num_segments = num_segments;
    memory->total_size = num_segments * memory->segment_size;
    
    return NULL;
}

#endif