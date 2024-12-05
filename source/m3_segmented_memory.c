#include "m3_segmented_memory.h"
#include "esp_log.h"

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
    memory->point = 0;

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
M3Result InitMemory(M3Memory* memory, size_t initial_stack, size_t initial_linear) {
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
    
    return M3Result_success;
}

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

// Funzione per la crescita dello stack
M3Result GrowStack(M3Memory* memory, size_t additional_size) {
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
    return M3Result_success;
}

/* // Example implementation

// Modifiche alle operazioni dello stack
d_m3Operation(Push_i32) {
    i32 value = *(i32*)(runtime->stack_pointer);
    
    // Verifica spazio disponibile
    if ((runtime->stack_pointer - sizeof(i32)) < 
        (runtime->memory.stack.base - runtime->memory.stack.size)) {
        if (GrowStack(&runtime->memory, PAGE_SIZE) != M3Result_success) {
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
    if (!new_segments) return M3Result_mallocFailed;
    
    memory->segments = new_segments;
    
    // Inizializza il nuovo segmento
    size_t new_idx = memory->num_segments;
    memory->segments[new_idx].data = m3_malloc(memory->segment_size);
    if (!memory->segments[new_idx].data) return M3Result_mallocFailed;
    
    memory->segments[new_idx].is_allocated = true;
    memory->num_segments++;
    memory->total_size += memory->segment_size;
    
    return M3Result_success;
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
M3Result GrowStack(M3Memory* memory, size_t additional_size) {
    size_t new_stack_size = memory->stack.size + additional_size;
    
    // Verifica se serve un nuovo segmento
    if (memory->linear.size + new_stack_size > memory->total_size) {
        M3Result result = AddSegment(memory);
        if (result != M3Result_success) return result;
        
        // Aggiorna i puntatori dello stack nella nuova configurazione
        memory->stack.base = (u8*)memory->segments[memory->num_segments-1].data + 
                            memory->segment_size;
    }
    
    memory->stack.size = new_stack_size;
    return M3Result_success;
}

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
M3Result InitMemory(M3Memory* memory, size_t initial_stack, size_t initial_linear) {
    memory->segment_size = WASM_SEGMENT_SIZE;  // es: 64KB (dimensione pagina WebAssembly)
    memory->num_segments = 1;  // Inizia con un segmento
    memory->segments = m3_malloc(sizeof(MemorySegment));
    if (!memory->segments) return M3Result_mallocFailed;
    
    // Alloca il primo segmento
    memory->segments[0].data = m3_malloc(memory->segment_size);
    if (!memory->segments[0].data) {
        m3_free(memory->segments);
        return M3Result_mallocFailed;
    }
    memory->segments[0].is_allocated = true;
    
    // Inizializza le regioni all'interno del primo segmento
    memory->linear.base = memory->segments[0].data;
    memory->linear.size = initial_linear;
    
    memory->stack.base = (u8*)memory->segments[0].data + memory->segment_size;
    memory->stack.size = initial_stack;
    
    memory->total_size = memory->segment_size;
    
    return M3Result_success;
}

M3Result ResizeMemory(IM3Runtime io_runtime, u32 i_numPages) {
    if(DEBUG_TOP_MEMORY) ESP_LOGI("WASM3", "ResizeMemory called");

    M3Memory* memory = &io_runtime->memory;

    if (i_numPages <= memory->maxPages) {
        size_t numPageBytes = (size_t)i_numPages * memory->pageSize;

        if (numPageBytes / memory->pageSize != i_numPages) {
            ESP_LOGE("WASM3", "Memory size calculation overflow!");
            return m3Err_mallocFailed;
        }

        #if d_m3MaxLinearMemoryPages > 0
        if (i_numPages > d_m3MaxLinearMemoryPages) {
            ESP_LOGE("WASM3", "linear memory limitation exceeded");
            return m3Err_mallocFailed;
        }
        #endif

        if (io_runtime->memoryLimit && numPageBytes > io_runtime->memoryLimit) {
            numPageBytes = io_runtime->memoryLimit;
            i_numPages = numPageBytes / memory->pageSize;
        }

        // Calcola il numero di segmenti necessari 
        // (un segmento per pagina, dato che segment_size = pageSize)
        size_t num_segments = i_numPages;

        // Gestione dei segmenti
        if (!memory->segments) {
            // Allocazione iniziale dell'array di strutture MemorySegment
            memory->segments = current_allocator->malloc(
                num_segments * sizeof(MemorySegment)
            );
            if (!memory->segments){ 
                ESP_LOGE("WASM3", "Failed memory->segment allocation");
                return m3Err_mallocFailed;
            }
            
            // Inizializza tutte le strutture dei segmenti
            for (size_t i = 0; i < num_segments; i++) {
                memory->segments[i].data = NULL;
                memory->segments[i].is_allocated = false;
                //memory->segment_size = memory->pageSize; // FUCK ME
            }
        }
        else if (num_segments != memory->num_segments) {
            // Rialloca l'array di strutture MemorySegment
            MemorySegment* new_segments = current_allocator->realloc(
                memory->segments,
                num_segments * sizeof(MemorySegment)
            );
            if (!new_segments){
                ESP_LOGE("WASM3", "Failed new_segments allocation");
                return m3Err_mallocFailed;
            }
            
            memory->segments = new_segments;

            // Se stiamo crescendo, inizializza i nuovi segmenti
            if (num_segments > memory->num_segments) {
                for (size_t i = memory->num_segments; i < num_segments; i++) {
                    memory->segments[i].data = NULL;
                    memory->segments[i].is_allocated = false;
                    //memory->segment_size = memory->pageSize;
                }
            }
            else {
                // Se stiamo riducendo, libera la memoria dei segmenti in eccesso
                for (size_t i = num_segments; i < memory->num_segments; i++) {
                    if (memory->segments[i].is_allocated && memory->segments[i].data) {
                        current_allocator->free(memory->segments[i].data);
                    }
                }
            }
        }

        // Aggiorna i metadati della memoria
        memory->num_segments = num_segments;
        memory->numPages = i_numPages;
        memory->total_size = numPageBytes;
        
        ESP_LOGI("WASM3", "Memory resized to %lu pages (%zu bytes, %zu segments)", 
                 i_numPages, numPageBytes, num_segments);
    }
    else {
        return m3Err_wasmMemoryOverflow;
    }

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

static inline void* GetMemorySegment(IM3Memory memory, u32 offset)
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
}

////////////////////////////////////////////////////////////////