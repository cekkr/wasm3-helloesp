#include "m3_segmented_memory.h"
#include "esp_log.h"

#include "m3_pointers.h"

IM3Memory m3_NewMemory(){
    IM3Memory memory = m3_Int_AllocStruct (M3Memory);
    memory->segment_size = WASM_SEGMENT_SIZE;

    if(memory == NULL){
        ESP_LOGE("WASM3", "m3_NewMemory: Memory allocation failed");
        return NULL;
    }

    memory->segments = NULL; //m3_NewArray (MemorySegment, WASM_MAX_SEGMENTS);    
    memory->max_size = 0; 
    memory->num_segments = 0;
    memory->total_size = 0;
    //memory->point = 0;

    // What are used for pages?
    memory->numPages = 0;
    memory->maxPages = M3Memory_MaxPages;

    return memory;
}

IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem){
    IM3MemoryPoint point = m3_Int_AllocStruct (M3MemoryPoint);

    if(point == NULL){
        ESP_LOGE("WASM3", "m3_GetMemoryPoint: Memory allocation failed");
        return NULL;
    }

    point->memory = mem;
    point->offset = 0;
    return point;
}

///
/// Memory fragmentation
///

// Funzioni di gestione della memoria
/*M3Result InitMemory(M3Memory* memory, size_t initial_stack, size_t initial_linear) {
    size_t total = initial_stack + initial_linear;
    memory->memory = m3_malloc(total);
    if (!memory->memory) return M3Result_mallocFailed;
    
    memory->total_size = total;
    
    // Inizializza la memoria lineare dal basso
    memory->linear.base = memory->memory;
    memory->linear.size = initial_linear;
    memory->linear.max_size = initial_linear * 2; // esempio
    
    // Inizializza lo stack dall'alto
    memory->stack.base = memory->memory + total;
    memory->stack.size = initial_stack;
    memory->stack.max_size = initial_stack * 2; // esempio
    
    // Stack pointer parte dall'alto
    memory->stack_pointer = memory->stack.base;
    
    return NULL;
}*/

// Funzione per verificare se un indirizzo appartiene allo stack
/*bool IsStackAddress(M3Memory* memory, u8* addr) {
    return (addr <= memory->stack.base && 
            addr >= (memory->stack.base - memory->stack.size));
}*/

// Funzione per verificare se un indirizzo appartiene alla memoria lineare
bool IsLinearAddress(M3Memory* memory, u8* addr) {
    return (addr >= memory->linear.base && 
            addr < (memory->linear.base + memory->linear.size));
}

// Funzione per convertire offset stack in indirizzo assoluto
u8* GetStackAddress(M3Memory* memory, size_t offset) {
    return memory->stack.base - offset;
}

// Funzione per la crescita dello stack
/*M3Result GrowStack(M3Memory* memory, size_t additional_size) {
    // Calcola nuovo punto di incontro
    size_t new_stack_size = memory->stack.size + additional_size;
    u8* new_stack_limit = memory->stack.base - new_stack_size;
    u8* linear_limit = memory->linear.base + memory->linear.size;
    
    // Verifica collisione
    if (new_stack_limit <= linear_limit) {
        // Necessaria riallocazione
        size_t new_total = memory->total_size * 2;
        u8* new_memory = m3_realloc(memory->memory, new_total);
        if (!new_memory) return M3Result_mallocFailed;
        
        // Aggiorna tutti i puntatori
        size_t stack_offset = memory->stack.base - memory->memory;
        size_t sp_offset = memory->stack_pointer - memory->memory;
        
        memory->memory = new_memory;
        memory->total_size = new_total;
        memory->stack.base = new_memory + stack_offset;
        memory->stack_pointer = new_memory + sp_offset;
        memory->linear.base = new_memory;
    }
    
    memory->stack.size = new_stack_size;
    return NULL;
}*/

/* // Example implementation

// Modifiche alle operazioni dello stack
d_m3Operation(Push_i32) {
    i32 value = *(i32*)(runtime->stack_pointer);
    
    // Verifica spazio disponibile
    if ((runtime->stack_pointer - sizeof(i32)) < 
        (runtime->memory.stack.base - runtime->memory.stack.size)) {
        if (GrowStack(&runtime->memory, PAGE_SIZE) != NULL) {
            return 0; // o gestione errore
        }
    }
    
    runtime->stack_pointer -= sizeof(i32);
    *(i32*)(runtime->stack_pointer) = value;
    return 0;
}

d_m3Operation(Pop_i32) {
    i32 value = *(i32*)(runtime->stack_pointer);
    runtime->stack_pointer += sizeof(i32);
    
    // Verifica overflow
    if (runtime->stack_pointer > runtime->memory.stack.base) {
        // Gestione errore stack overflow
        return 0;
    }
    
    return (m3reg_t)value;
}
*/

///
/// Stack/linear differation
///

// Funzione per aggiungere un nuovo segmento
M3Result AddSegment(M3Memory* memory) {
    size_t new_size = (memory->num_segments + 1) * sizeof(MemorySegment);
    MemorySegment* new_segments = m3_realloc(memory->segments, new_size);
    if (!new_segments) return m3Err_mallocFailed;
    
    memory->segments = new_segments;
    
    // Inizializza il nuovo segmento
    size_t new_idx = memory->num_segments;
    memory->segments[new_idx].data = m3_malloc(memory->segment_size);
    if (!memory->segments[new_idx].data) return m3Err_mallocFailed;
    
    memory->segments[new_idx].is_allocated = true;
    memory->num_segments++;
    memory->total_size += memory->segment_size;
    
    return NULL; // aka success
}

// Funzione per trovare l'indirizzo effettivo dato un offset
u8* GetEffectiveAddress(M3Memory* memory, size_t offset) {
    size_t segment_idx = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    
    if (segment_idx >= memory->num_segments) return NULL;
    
    return (u8*)memory->segments[segment_idx].data + segment_offset;
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

// Funzione per la crescita dello stack
/*M3Result GrowStack(M3Memory* memory, size_t additional_size) {
    size_t new_stack_size = memory->stack.size + additional_size;
    
    // Verifica se serve un nuovo segmento
    if (memory->linear.size + new_stack_size > memory->total_size) {
        M3Result result = AddSegment(memory);
        if (result != NULL) return result;
        
        // Aggiorna i puntatori dello stack nella nuova configurazione
        memory->stack.base = (u8*)memory->segments[memory->num_segments-1].data + 
                            memory->segment_size;
    }
    
    memory->stack.size = new_stack_size;
    return NULL;
}*/

///
/// From m3_env
///

static const int DEBUG_TOP_MEMORY = 1;

/*
// old implementation. even older ones in m3_env
M3Result InitMemory(IM3Runtime io_runtime, IM3Module i_module) {
    if(DEBUG_TOP_MEMORY) ESP_LOGI("WASM3", "InitMemory called");

    if (i_module->memoryInfo.pageSize == 0) {
        i_module->memoryInfo.pageSize = 65536;  // Standard WebAssembly page size
        ESP_LOGI("WASM3", "InitMemory - Fixed pageSize to standard 64KB");
    }

    M3Result result = m3Err_none;

    if (!i_module->memoryImported) {
        io_runtime->memory.maxPages = i_module->memoryInfo.maxPages ? 
                                    i_module->memoryInfo.maxPages : 65536;
        io_runtime->memory.pageSize = i_module->memoryInfo.pageSize;
        io_runtime->memory.segment_size = WASM_SEGMENT_SIZE;  // Usa la page size come segment size
        
        result = ResizeMemory(io_runtime, i_module->memoryInfo.initPages);
    }
    else {
        if(io_runtime->memory.segment_size == 0) {
            ESP_LOGW("WASM3", "InitMemory: io_runtime->memory.segment_size == 0");
            io_runtime->memory.segment_size = WASM_SEGMENT_SIZE;
        }
    }

    return result;
}*/

// Trying to implement stack/linear memory differentiation
/*M3Result InitMemory(M3Memory* memory, size_t initial_stack, size_t initial_linear) {
    memory->segment_size = WASM_SEGMENT_SIZE;  // es: 64KB (dimensione pagina WebAssembly)
    memory->num_segments = 1;  // Inizia con un segmento
    memory->segments = m3_malloc(sizeof(MemorySegment));
    if (!memory->segments) return m3Err_mallocFailed;
    
    // Alloca il primo segmento
    memory->segments[0].data = m3_malloc(memory->segment_size);
    if (!memory->segments[0].data) {
        m3_free(memory->segments);
        return m3Err_mallocFailed;
    }
    memory->segments[0].is_allocated = true;
    
    // Inizializza le regioni all'interno del primo segmento
    memory->linear.base = memory->segments[0].data;
    memory->linear.size = initial_linear;
    
    memory->stack.base = (u8*)memory->segments[0].data + memory->segment_size;
    memory->stack.size = initial_stack;
    
    memory->total_size = memory->segment_size;
    
    return NULL; // aka success
}*/

M3Result ResizeMemory(IM3Runtime io_runtime, u32 i_numPages) {
    if (!io_runtime) return m3Err_nullRuntime;
    
    M3Memory* memory = &io_runtime->memory;
    if (!memory) return m3Err_mallocFailed;

    // Validate page count against maximum
    if (i_numPages > memory->maxPages) {
        return m3Err_wasmMemoryOverflow;
    }

    // Calculate new total size in bytes
    size_t newPageBytes = (size_t)i_numPages * memory->pageSize;
    if (newPageBytes / memory->pageSize != i_numPages) {
        return m3Err_mallocFailed;  // Overflow check
    }

    #if d_m3MaxLinearMemoryPages > 0
    if (i_numPages > d_m3MaxLinearMemoryPages) {
        return m3Err_mallocFailed;
    }
    #endif

    // Apply memory limit if set
    if (io_runtime->memoryLimit && newPageBytes > io_runtime->memoryLimit) {
        newPageBytes = io_runtime->memoryLimit;
        i_numPages = newPageBytes / memory->pageSize;
    }

    // Calculate required segments based on new size
    size_t new_num_segments = (newPageBytes + memory->segment_size - 1) / memory->segment_size;
    
    // Handle initial allocation
    if (!memory->segments) {
        memory->segments = current_allocator->malloc(new_num_segments * sizeof(MemorySegment));
        if (!memory->segments) {
            return m3Err_mallocFailed;
        }
        
        // Initialize new segments
        for (size_t i = 0; i < new_num_segments; i++) {
            memory->segments[i].data = NULL;
            memory->segments[i].is_allocated = false;
            memory->segments[i].stack_size = 0;
            memory->segments[i].linear_size = 0;
        }
    }
    // Handle resize
    else if (new_num_segments != memory->num_segments) {
        // Reallocate segment array
        MemorySegment* new_segments = current_allocator->realloc(
            memory->segments,
            new_num_segments * sizeof(MemorySegment)
        );
        
        if (!new_segments) {
            return m3Err_mallocFailed;
        }
        
        memory->segments = new_segments;

        if (new_num_segments > memory->num_segments) {
            // Initialize newly added segments
            for (size_t i = memory->num_segments; i < new_num_segments; i++) {
                memory->segments[i].data = NULL;
                memory->segments[i].is_allocated = false;
                memory->segments[i].stack_size = 0;
                memory->segments[i].linear_size = 0;
            }
        } else {
            // Free memory from removed segments
            for (size_t i = new_num_segments; i < memory->num_segments; i++) {
                if (memory->segments[i].is_allocated && memory->segments[i].data) {
                    current_allocator->free(memory->segments[i].data);
                    memory->segments[i].is_allocated = false;
                    memory->segments[i].data = NULL;
                }
            }
        }
    }

    // Update memory regions
    size_t total_linear_size = newPageBytes * 3/4;  // 75% for linear memory
    size_t total_stack_size = newPageBytes - total_linear_size;  // 25% for stack

    // Update linear memory region
    memory->linear.size = total_linear_size;
    if (memory->linear.current_offset > total_linear_size) {
        memory->linear.current_offset = total_linear_size;
    }

    // Update stack region
    memory->stack.size = total_stack_size;
    if (memory->stack.current_offset > total_stack_size) {
        memory->stack.current_offset = total_stack_size;
    }
    
    // Update memory metadata
    memory->num_segments = new_num_segments;
    memory->numPages = i_numPages;
    memory->total_size = newPageBytes;

    return m3Err_none;
}


void FreeMemory(IM3Runtime io_runtime) {
    if(DEBUG_TOP_MEMORY) ESP_LOGI("WASM3", "FreeMemory called");
    
    M3Memory* memory = &io_runtime->memory;

    if (memory->segments) {
        // Libera la memoria di ogni segmento allocato
        for (size_t i = 0; i < memory->num_segments; i++) {
            if (memory->segments[i].is_allocated && memory->segments[i].data) {
                m3_Free_Impl(memory->segments[i].data, false);
            }
        }
        // Libera l'array dei segmenti
        m3_Free_Impl(memory->segments, false);
        memory->segments = NULL;
        memory->num_segments = 0;
    }

    memory->numPages = 0;
    memory->maxPages = 0;
}


////////////////////////////////////////////////////////////////
// Segmented memory management

#define WASM_DEBUG_SEGMENTED_MEM_MAN 1

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

/*static inline void* GetMemorySegment(IM3Memory memory, u32 offset)
{
    size_t segment_index = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    MemorySegment segment = memory->segments[segment_index];
    return (u8*)segment.data + segment_offset;
}

static inline i32 m3_LoadInt(IM3Memory memory, u32 offset)
{
    void* ptr = GetMemorySegment(memory, offset);
    return *(i32*)ptr;
}

static inline void m3_StoreInt(IM3Memory memory, u32 offset, i32 value)
{
    void* ptr = GetMemorySegment(memory, offset);
    *(i32*)ptr = value;
}*/

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
    
    memory->segments = m3_malloc(num_segments * sizeof(MemorySegment));
    if (!memory->segments) return M3Result_mallocFailed;
    
    // Initialize segments
    for (size_t i = 0; i < num_segments; i++) {
        memory->segments[i].data = NULL;
        memory->segments[i].is_allocated = false;
        memory->segments[i].stack_size = 0;
        memory->segments[i].linear_size = 0;
    }
    
    // Allocate first segment
    if (!allocate_segment(memory, 0)) {
        m3_free(memory->segments);
        return M3Result_mallocFailed;
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

// Memory access helpers
static inline void* GetMemorySegment(IM3Memory memory, u32 offset) {
    u8* addr = GetEffectiveAddress(memory, offset);
    if (!addr) {
        // Handle error - could throw exception or return error code
        return NULL;
    }
    return addr;
}

static inline i32 m3_LoadInt(IM3Memory memory, u32 offset) {
    void* ptr = GetMemorySegment(memory, offset);
    if (!ptr) return 0; // Or handle error
    return *(i32*)ptr;
}

static inline void m3_StoreInt(IM3Memory memory, u32 offset, i32 value) {
    void* ptr = GetMemorySegment(memory, offset);
    if (ptr) {
        *(i32*)ptr = value;
    }
    // else handle error
}