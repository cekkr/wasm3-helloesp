#include "m3_segmented_memory.h"
#include "esp_log.h"
#include "m3_pointers.h"

#define WASM_DEBUG_SEGMENTED_MEM_MAN 1


#define PRINT_PTR(ptr) ESP_LOGI("WASM3", "Pointer value: (unsigned: %u, signed: %d)", (uintptr_t)ptr, (intptr_t)ptr)

#define MIN(x, y) ((x) < (y) ? (x) : (y)) 

const bool WASM_DEBUG_SEGMENTED_MEMORY_ALLOC = true;

const bool DEBUG_WASM_INIT_MEMORY = true;
// Utility functions
static bool is_address_in_segment(MemorySegment* seg, void* ptr) {
    if (!seg || !seg->data || !ptr) return false;
    return (ptr >= seg->data && ptr < (char*)seg->data + seg->size);
}

static size_t get_segment_index(M3Memory* memory, void* ptr) {
    for (size_t i = 0; i < memory->num_segments; i++) {
        if (is_address_in_segment(memory->segments[i], ptr)) return i;
    }
    return (size_t)-1;
}

////////////////////////////////////////////////////////////////////////

IM3MemoryPoint m3_GetMemoryPoint(IM3Memory mem){
    IM3MemoryPoint res = m3_Def_AllocStruct(M3MemoryPoint);
    if(res != NULL){
        res->memory = mem;
        res->firm = 20190394;
    }
    return res;
}

IM3MemoryPoint ValidateMemoryPoint(void* ptr) {
    if (ptr == NULL) {
        return NULL;
    }

    if(!is_ptr_valid(ptr)){
        return NULL;
    }
    
    IM3MemoryPoint point = (IM3MemoryPoint)ptr;
    
    // Verifica la firma
    if (point->firm != M3PTR_FIRM) {
        return NULL;
    }
    
    return point;
}

////////////////////////////////////////////////////////////////

const bool WASM_DEBUG_GET_OFFSET_POINTER = true;
mos get_offset_pointer(IM3Memory memory, void* ptr) {
    if (!memory || memory->firm != INIT_FIRM || !memory->segments || !ptr) {
        if(WASM_DEBUG_GET_OFFSET_POINTER) ESP_LOGW("WASM3", "get_offset_pointer: null memory or invalid ptr");
        return (mos)ptr;
    }

    // If ptr is already an offset or ERROR_POINTER, return it as is
    if(WASM_DEBUG_GET_OFFSET_POINTER) ESP_LOGW("WASM3", "get_offset_pointer: ptr= %d, memory->total_size= %d", (mos)ptr, memory->total_size);
    if (ptr == ERROR_POINTER || (mos)ptr < memory->total_size) {
        return (mos)ptr;
    }

    // Search through all segments to find which one contains this pointer
    for (size_t i = 0; i < memory->num_segments; i++) {
        MemorySegment* segment = memory->segments[i];
        if (!segment || !segment->data) continue;

        // Calculate if ptr falls within this segment's range
        void* segment_start = segment->data;
        void* segment_end = (uint8_t*)segment_start + segment->size;

        if (ptr >= segment_start && ptr < segment_end) {
            // Calculate the offset within the segment
            size_t segment_offset = (uint8_t*)ptr - (uint8_t*)segment_start;
            
            // Calculate the total offset
            mos total_offset = (i * memory->segment_size) + segment_offset;

            if (WASM_DEBUG_GET_OFFSET_POINTER) {
                ESP_LOGI("WASM3", "get_offset_pointer: converted %p to offset %u (segment %zu)", 
                         ptr, total_offset, i);
            }

            return total_offset;
        }
    }

    // If we didn't find the pointer in any segment, return the original pointer
    if (WASM_DEBUG_GET_OFFSET_POINTER) {
        ESP_LOGI("WASM3", "get_offset_pointer: pointer %p not found in segmented memory", ptr);
    }
    
    return (mos)ptr;
}

// Core pointer resolution functions
bool WASM_DEBUG_get_offset_pointer = true;
void* get_segment_pointer(IM3Memory memory, u32 offset) {
    if(WASM_DEBUG_get_offset_pointer) ESP_LOGI("WASM3", "get_segment_pointer called with offset %u", offset);

    if (!memory || memory->firm != INIT_FIRM) {
        return ERROR_POINTER;
    }
    
    // Calculate segment indices
    size_t segment_index = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    
    // Validate segment
    if (segment_index >= memory->num_segments) {
        // Try to grow memory if needed
        if (segment_index - memory->num_segments < 3) {
            if (AddSegments(memory, segment_index + 1 - memory->num_segments) != NULL) {
                return ERROR_POINTER;
            }
        } else {
            return ERROR_POINTER;
        }
    }
    
    MemorySegment* seg = memory->segments[segment_index];
    if (!seg || seg->firm != INIT_FIRM) return ERROR_POINTER;
    
    // Initialize segment if needed
    if (!seg->data) {
        if(WASM_DEBUG_get_offset_pointer) ESP_LOGI("WASM3", "get_segment_pointer: requested data allocation of segment %d", segment_index);
        seg = InitSegment(memory, seg, true);

        if(seg == NULL)
            return ERROR_POINTER;
    }
    
    // Handle multi-segment chunks
    MemoryChunk* chunk = seg->first_chunk;
    while (chunk) {
        if (chunk->num_segments > 1) {
            size_t chunk_start = chunk->start_segment * memory->segment_size;
            size_t chunk_end = chunk_start;
            for (size_t i = 0; i < chunk->num_segments; i++) {
                chunk_end += chunk->segment_sizes[i];
            }
            
            if (offset >= chunk_start && offset < chunk_end) {
                // Calculate which segment of the chunk we're in
                size_t relative_offset = offset - chunk_start;
                size_t current_size = 0;
                for (size_t i = 0; i < chunk->num_segments; i++) {
                    if (relative_offset < current_size + chunk->segment_sizes[i]) {
                        return (void*)((char*)memory->segments[chunk->start_segment + i]->data + 
                                     (relative_offset - current_size));
                    }
                    current_size += chunk->segment_sizes[i];
                }
            }
        }
        chunk = chunk->next;
    }
    
    return (void*)((char*)seg->data + segment_offset);
}

void* resolve_pointer(M3Memory* memory, void* ptr) {
    if (is_ptr_valid(ptr)) return ptr;
    
    if (!memory || memory->firm != INIT_FIRM) return ptr;
    
    u32 offset = (u32)ptr;
    if (offset >= memory->total_size) return ptr;
    
    void* resolved = get_segment_pointer(memory, offset);
    if (resolved == ERROR_POINTER) return ptr;
    
    return resolved;
}

int find_segment_index(MemorySegment** segments, int num_segments, MemorySegment* segment) {
    for (int i = 0; i < num_segments; i++) {
        if (segments[i] == segment) {
            return i;
        }
    }
    return -1;  // Se non viene trovato
}

// Memory initialization and growth
const bool WASM_DEBUG_INITSEGMENT = true;
MemorySegment* InitSegment(M3Memory* memory, MemorySegment* seg, bool initData) {
    if (memory == NULL || memory->firm != INIT_FIRM){ 
        ESP_LOGW("WASM", "InitSegment: memory not initialized");
        return NULL;
    }
    
    if (seg == NULL) {
        ESP_LOGW("WASM", "InitSegment: segment allocation on the run");
        seg = m3_Def_Malloc(sizeof(MemorySegment));
        if (seg == NULL) {
            ESP_LOGW("WASM", "InitSegment: failed to allocate memory segment");
            return NULL;
        }

        seg->index = find_segment_index(memory->segments, memory->num_segments, seg);
    }
    
    seg->firm = INIT_FIRM;
    
    if (initData && !seg->data) {
        if(WASM_DEBUG_INITSEGMENT) ESP_LOGI("WASM", "InitSegment: allocating segment's data");
        seg->data = m3_Def_Malloc(memory->segment_size);
        if (seg->data == NULL) {
            ESP_LOGE("WASM", "InitSegment: segmente data allocate failed");
            return NULL;
        }
        
        seg->is_allocated = true;
        seg->size = memory->segment_size;
        seg->first_chunk = NULL;
        memory->total_allocated_size += memory->segment_size;
    }
    
    return seg;
}

const bool WASM_DEBUG_ADDSEGMENT = true;
M3Result AddSegments(IM3Memory memory, size_t additional_segments) {
    if(WASM_DEBUG_ADDSEGMENT) ESP_LOGI("WASM3", "AddSegments: requested %zu segments", additional_segments);
    if (memory == NULL || memory->firm != INIT_FIRM) {
        ESP_LOGE("WASM3", "AddSegments: memory is not initialized");
        return m3Err_nullMemory;    
    }

    size_t new_num_segments = additional_segments == 0 ? (memory->num_segments + 1) : additional_segments;

    if(new_num_segments <= memory->num_segments) {
        if(WASM_DEBUG_ADDSEGMENT) ESP_LOGW("WASM3", "AddSegments: no needed to add more segments (req: %d, current: %d)", new_num_segments, memory->num_segments);
        return NULL;
    }

    size_t new_size = new_num_segments * sizeof(MemorySegment*);
    
    MemorySegment** new_segments = m3_Def_Realloc(memory->segments, new_size);
    if (!new_segments) {
        ESP_LOGE("WASM3", "AddSegments: realloc memory->segments failed");
        return m3Err_mallocFailed;
    }
    
    memory->segments = new_segments;
    
    // Initialize new segments
    for (size_t i = memory->num_segments; i < new_num_segments; i++) {
        memory->segments[i] = m3_Def_Malloc(sizeof(MemorySegment));

        if (!memory->segments[i]) {
            // Rollback on failure
            for (size_t j = memory->num_segments; j < i; j++) {
                m3_Def_Free(memory->segments[j]);
            }
            return m3Err_mallocFailed;
        }

        memory->segments[i]->index = i;
        
        void* seg = InitSegment(memory, memory->segments[i], false);
        if (seg == NULL) {
            ESP_LOGE("WASM3", "AddSegment: InitSegment %d failed", i);

            // Cleanup on failure
            for (size_t j = memory->num_segments; j <= i; j++) {
                m3_Def_Free(memory->segments[j]);
            }
            return m3Err_mallocFailed;
        }
    }
    
    memory->num_segments = new_num_segments;
    memory->total_size = memory->segment_size * new_num_segments;
    
    return m3Err_none;
}

const bool WASM_DEBUG_M3_INIT_MEMORY = true;
const int WASM_M3_INIT_MEMORY_NUM_MALLOC_TESTS = 10;
IM3Memory m3_InitMemory(IM3Memory memory) {
    if (memory == NULL) return NULL;

    if(memory->firm == INIT_FIRM)
        return memory;
    
    memory->firm = INIT_FIRM;
    memory->segments = NULL;
    memory->num_segments = 0;
    memory->total_size = 0;
    memory->total_allocated_size = 0;
    memory->segment_size = WASM_SEGMENT_SIZE;
    memory->maxPages = M3Memory_MaxPages;
    memory->pageSize = M3Memory_PageSize;
    memory->total_requested_size = 0;
    
    // Initialize free chunks management
    memory->num_free_buckets = 32;
    memory->free_chunks = m3_Def_Malloc(memory->num_free_buckets * sizeof(MemoryChunk*));
    if (!memory->free_chunks) return NULL;
    
    // Add initial segments
    M3Result result = AddSegments(memory, WASM_INIT_SEGMENTS);
    if (result != m3Err_none) {
        ESP_LOGE("WASM3", "m3_InitMemory: AddSegments failed");
        free(memory->free_chunks);
        return NULL;
    }


    if(WASM_M3_INIT_MEMORY_NUM_MALLOC_TESTS > 0){
        for(int i = 0; i < WASM_M3_INIT_MEMORY_NUM_MALLOC_TESTS; i++){
            ESP_LOGI("WASM3", "m3_InitMemory: test m3_malloc num %d", i);
            void* testPtr = m3_malloc(memory, 1);
            PRINT_PTR(testPtr);
        }
    }
    
    return memory;
}

IM3Memory m3_NewMemory(){
    IM3Memory memory = m3_Def_AllocStruct (M3Memory);

    m3_InitMemory(memory);

    return memory;
}

bool IsValidMemory(IM3Memory memory) {
    if(memory == NULL || memory->firm != INIT_FIRM) return false;
    return memory;
}

M3Result GrowMemory(M3Memory* memory, size_t additional_size) {
    if (!memory) return m3Err_nullMemory;
    
    size_t new_total = memory->total_size + additional_size;
    if (new_total > memory->maxPages * WASM_SEGMENT_SIZE) {
        return m3Err_memoryLimit;
    }
    
    size_t additional_segments = (additional_size + WASM_SEGMENT_SIZE - 1) / WASM_SEGMENT_SIZE;
    return AddSegments(memory, additional_segments);
}

// Memory operations
const bool WASM_DEBUG_IsValidMemoryAccess = true;
bool IsValidMemoryAccess(IM3Memory memory, mos offset, size_t size) {
    if(WASM_DEBUG_IsValidMemoryAccess) ESP_LOGI("WASM3", "IsValidMemoryAccess called with memory=%p, offset=%p, size=%d", memory, offset, size);

    if (!memory || !memory->segments) goto isNotSegMem;

    if(WASM_DEBUG_IsValidMemoryAccess) {
        ESP_LOGI("WASM3", "IsValidMemoryAccess: memory->total_size=%zu, offset=%zu", memory->total_size, offset);
    }    

    if (offset + size > memory->total_size) goto isNotSegMem;
    
    size_t start_segment = offset / memory->segment_size;
    size_t end_segment = (offset + size - 1) / memory->segment_size;
    
    // Verify all needed segments exist and are initialized
    for (size_t i = start_segment; i <= end_segment; i++) {
        if (i >= memory->num_segments) goto isNotSegMem;
        
        MemorySegment* seg = memory->segments[i];
        if (!seg || !seg->data) goto isNotSegMem;
        
        // Check for multi-segment chunks
        if (seg->first_chunk) {
            MemoryChunk* chunk = seg->first_chunk;
            while (chunk) {
                if (chunk->num_segments > 1) {
                    size_t chunk_start = chunk->start_segment * memory->segment_size;
                    size_t chunk_end = chunk_start;
                    for (size_t j = 0; j < chunk->num_segments; j++) {
                        chunk_end += chunk->segment_sizes[j];
                    }
                    
                    if (offset >= chunk_start && offset + size <= chunk_end) {
                        return true;  // Access is within a valid multi-segment chunk
                    }
                }
                chunk = chunk->next;
            }
        }
    }
    
    return true;

    isNotSegMem:

    void* ptr = (void*)offset;
    if(!is_ptr_valid(ptr)){
        ESP_LOGW("WASM3", "IsValidMemoryAccess: is not segmented pointer, and both not valid pointer");
        LOG_FLUSH; LOG_FLUSH;
        backtrace();
    }

    return false;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////


static u32 ptr_to_offset(M3Memory* memory, void* ptr) {
    if (!memory || !ptr) return 0;
    
    for (size_t i = 0; i < memory->num_segments; i++) {
        MemorySegment* seg = memory->segments[i];
        if (!seg || !seg->data) continue;
        
        // Verifica se il puntatore è all'interno di questo segmento
        if (ptr >= seg->data && ptr < (void*)((char*)seg->data + seg->size)) {
            // Se il puntatore è all'interno del segmento, calcola l'offset
            size_t segment_base_offset = i * memory->segment_size;
            size_t intra_segment_offset = (char*)ptr - (char*)seg->data;
            
            // Se stiamo ritornando un offset per i dati (non per il MemoryChunk stesso)
            // dobbiamo sottrarre la dimensione dell'header dal calcolo dell'offset
            if (intra_segment_offset >= sizeof(MemoryChunk)) {
                size_t data_offset = intra_segment_offset - sizeof(MemoryChunk);
                return segment_base_offset + data_offset;
            }
            
            return segment_base_offset + intra_segment_offset;
        }
    }
    
    return 0;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void* m3_malloc(M3Memory* memory, size_t size) {
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_malloc: entering with size=%zu", size);
    }

    if (!memory || memory->firm != INIT_FIRM || size == 0) {
        ESP_LOGE("WASM3", "m3_malloc: invalid arguments - memory=%p, firm=%d, size=%zu", 
                 memory, memory ? memory->firm : 0, size);
        return NULL;
    }
    
    size_t total_size = size + sizeof(MemoryChunk);
    // Allinea la dimensione totale a WASM_CHUNK_SIZE
    total_size = (total_size + (WASM_CHUNK_SIZE-1)) & ~(WASM_CHUNK_SIZE-1);
    
    size_t bucket = log2(total_size);
    MemoryChunk* found_chunk = NULL;    
    
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_malloc: calculated bucket=%zu for total_size=%zu (aligned from %zu)", 
                 bucket, total_size, size);
    }
    
    // 1. Prima cerca nei chunk liberi
    if (bucket < memory->num_free_buckets) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_malloc: searching in free chunks bucket %zu", bucket);
        }
        
        MemoryChunk** curr = &memory->free_chunks[bucket];
        while (*curr) {
            if ((*curr)->size >= total_size) {
                found_chunk = *curr;
                *curr = found_chunk->next;
                if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                    ESP_LOGI("WASM3", "m3_malloc: found free chunk of size=%zu at segment=%zu", 
                             found_chunk->size, found_chunk->start_segment);
                }
                break;
            }
            curr = &(*curr)->next;
        }
    }
    
    // 2. Se non trova chunk liberi, cerca spazio alla fine dei segmenti esistenti
    if (!found_chunk) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_malloc: no free chunks found, searching in existing segments");
        }
            
        bool space_found = false;
        
        for (size_t i = 0; i < memory->num_segments && !space_found; i++) {
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_malloc: examining segment %zu (%p)", i, (void*)memory->segments[i]);
            }
                
            MemorySegment* seg = memory->segments[i];
            if (!seg || !seg->data) {
                if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                    ESP_LOGI("WASM3", "m3_malloc: initializing segment %zu", i);
                }
                if (InitSegment(memory, seg, true) == NULL) {
                    ESP_LOGE("WASM3", "m3_malloc: failed to initialize segment %zu", i);
                    continue;
                }
            }
            
            // Calcola spazio utilizzato nel segmento
            size_t used_space = 0;
            MemoryChunk* current = seg->first_chunk;
            MemoryChunk* last_chunk = NULL;
            
            // Traccia tutti i chunk nel segmento
            while (current) {
                used_space += current->size;
                last_chunk = current;
                current = current->next;
            }
            
            // Allinea used_space a WASM_CHUNK_SIZE
            used_space = (used_space + (WASM_CHUNK_SIZE-1)) & ~(WASM_CHUNK_SIZE-1);
            
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_malloc: segment %zu analysis - used_space=%zu, total_size=%zu, available=%zu", 
                         i, used_space, seg->size, seg->size - used_space);
            }
            
            // Se c'è spazio sufficiente dopo l'ultimo chunk
            if (used_space + total_size <= seg->size) {
                found_chunk = (MemoryChunk*)((char*)seg->data + used_space);
                memset(found_chunk, 0, sizeof(MemoryChunk));  // Zero initialize
                
                found_chunk->size = total_size;
                found_chunk->is_free = false;
                found_chunk->next = NULL;
                found_chunk->prev = last_chunk;
                found_chunk->num_segments = 1;
                found_chunk->start_segment = i;
                found_chunk->segment_sizes = m3_Def_Malloc(sizeof(size_t));
                
                if (!found_chunk->segment_sizes) {
                    ESP_LOGE("WASM3", "m3_malloc: failed to allocate segment_sizes array");
                    return NULL;
                }
                
                found_chunk->segment_sizes[0] = total_size;
                
                if (last_chunk) {
                    last_chunk->next = found_chunk;
                } else {
                    seg->first_chunk = found_chunk;
                }
                
                if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                    ESP_LOGI("WASM3", "m3_malloc: allocated new chunk in segment %zu at offset %zu", 
                             i, used_space);
                }
                
                space_found = true;
                break;
            }
        }
    }
    
    // 3. Se non trova spazio nei segmenti esistenti, alloca nuovi segmenti
    if (!found_chunk) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_malloc: no space in existing segments, allocating new segments");
        }
        
        // Calcola quanti segmenti servono
        size_t segments_needed = (total_size + memory->segment_size - 1) / memory->segment_size;
        if (segments_needed == 0) segments_needed = 1;
        
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_malloc: need %zu new segments", segments_needed);
        }
        
        // Aggiungi i segmenti necessari
        M3Result result = AddSegments(memory, segments_needed);
        if (result != NULL) {
            ESP_LOGE("WASM3", "m3_malloc: failed to add %zu new segments", segments_needed);
            return NULL;
        }
        
        // Usa il primo nuovo segmento
        size_t new_segment_index = memory->num_segments - segments_needed;
        MemorySegment* new_seg = memory->segments[new_segment_index];
        
        if (!new_seg->data) {
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_malloc: initializing new segment %zu", new_segment_index);
            }
            if (InitSegment(memory, new_seg, true) != NULL) {
                ESP_LOGE("WASM3", "m3_malloc: failed to initialize new segment %zu", new_segment_index);
                return NULL;
            }
        }
        
        found_chunk = (MemoryChunk*)new_seg->data;
        memset(found_chunk, 0, sizeof(MemoryChunk));  // Zero initialize
        found_chunk->size = total_size;
        found_chunk->is_free = false;
        found_chunk->num_segments = segments_needed;
        found_chunk->start_segment = new_segment_index;
        found_chunk->segment_sizes = m3_Def_Malloc(sizeof(size_t) * segments_needed);
        
        if (!found_chunk->segment_sizes) {
            ESP_LOGE("WASM3", "m3_malloc: failed to allocate segment_sizes array for %zu segments", 
                     segments_needed);
            return NULL;
        }
        
        // Distribuisci la size tra i segmenti
        size_t remaining_size = total_size;
        for (size_t i = 0; i < segments_needed; i++) {
            size_t available = memory->segment_size;
            if (i == 0) available -= sizeof(MemoryChunk);
            found_chunk->segment_sizes[i] = (remaining_size > available) ? available : remaining_size;
            remaining_size -= found_chunk->segment_sizes[i];
            
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_malloc: segment %zu allocated %zu bytes", 
                         new_segment_index + i, found_chunk->segment_sizes[i]);
            }
        }
        
        new_seg->first_chunk = found_chunk;
    }
    
    if (found_chunk) {
        // Calcola l'offset relativo al primo segmento
        u32 base_offset = found_chunk->start_segment * memory->segment_size;
        u32 chunk_offset = (char*)found_chunk - (char*)memory->segments[found_chunk->start_segment]->data;
        u32 offset = base_offset + chunk_offset + sizeof(MemoryChunk);
        
        // Verifica che l'offset sia allineato
        if (offset % WASM_CHUNK_SIZE != 0) {
            offset = (offset + (WASM_CHUNK_SIZE-1)) & ~(WASM_CHUNK_SIZE-1);
        }
        
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_malloc: returning offset=%u (segment=%u, chunk_offset=%u)", 
                     offset, found_chunk->start_segment, chunk_offset);
        }
        
        memory->total_requested_size += size;
        return (void*)offset;
    }
    
    ESP_LOGE("WASM3", "m3_malloc: allocation failed for size %zu", size);
    return NULL;
}

void m3_free(M3Memory* memory, void* offset_ptr) {
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_free: entering with ptr=%p", offset_ptr);
    }

    if (!memory || !offset_ptr) {
        ESP_LOGE("WASM3", "m3_free: invalid arguments - memory=%p, ptr=%p", memory, offset_ptr);
        return;
    }
    
    // Converti l'offset in puntatore al chunk
    u32 offset = (u32)offset_ptr;
    if (offset < sizeof(MemoryChunk)) {
        ESP_LOGE("WASM3", "m3_free: invalid offset %u (smaller than MemoryChunk)", offset);
        return;
    }

    // Calcola indici di segmento
    size_t segment_index = (offset - sizeof(MemoryChunk)) / memory->segment_size;
    if (segment_index >= memory->num_segments) {
        ESP_LOGE("WASM3", "m3_free: segment index %zu out of bounds (max %zu)", 
                 segment_index, memory->num_segments - 1);
        return;
    }
    
    MemorySegment* seg = memory->segments[segment_index];
    if (!seg || !seg->data) {
        ESP_LOGE("WASM3", "m3_free: invalid segment at index %zu", segment_index);
        return;
    }
    
    size_t intra_segment_offset = (offset - sizeof(MemoryChunk)) % memory->segment_size;
    MemoryChunk* chunk = (MemoryChunk*)((char*)seg->data + intra_segment_offset - sizeof(MemoryChunk));
    
    // Verifica validità del chunk
    if (chunk->start_segment != segment_index || !chunk->segment_sizes) {
        ESP_LOGE("WASM3", "m3_free: invalid chunk (start_segment=%zu, expected=%zu)", 
                 chunk->start_segment, segment_index);
        return;
    }

    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_free: freeing chunk at segment %zu with %zu segments, size=%zu", 
                 segment_index, chunk->num_segments, chunk->size);
    }
    
    // Gestione chunk multi-segmento
    if (chunk->num_segments > 1) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_free: processing multi-segment chunk");
        }
        
        // Rimuovi il chunk dalle liste di tutti i segmenti che occupa
        for (size_t i = 0; i < chunk->num_segments; i++) {
            size_t curr_segment_index = chunk->start_segment + i;
            if (curr_segment_index >= memory->num_segments) {
                ESP_LOGE("WASM3", "m3_free: segment index %zu out of bounds", curr_segment_index);
                break;
            }
            
            MemorySegment* curr_seg = memory->segments[curr_segment_index];
            if (!curr_seg) continue;
            
            // Se questo è il primo chunk nel segmento, aggiorna first_chunk
            if (curr_seg->first_chunk == chunk) {
                if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                    ESP_LOGI("WASM3", "m3_free: updating first_chunk for segment %zu", 
                             curr_segment_index);
                }
                curr_seg->first_chunk = chunk->next;
            }
            
            // Aggiorna i collegamenti per gli altri chunk nel segmento
            if (i == 0) { // Solo nel primo segmento gestiamo i link prev/next
                if (chunk->prev) chunk->prev->next = chunk->next;
                if (chunk->next) chunk->next->prev = chunk->prev;
            }
        }
    } else {
        // Gestione chunk single-segment
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_free: processing single-segment chunk");
        }
        
        if (chunk->prev) {
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_free: updating prev chunk link");
            }
            chunk->prev->next = chunk->next;
        } else {
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_free: updating first_chunk for segment %zu", segment_index);
            }
            seg->first_chunk = chunk->next;
        }
        if (chunk->next) {
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_free: updating next chunk link");
            }
            chunk->next->prev = chunk->prev;
        }
    }
    
    // Gestisci la fusione con chunk adiacenti liberi
    MemoryChunk* next = chunk->next;
    MemoryChunk* prev = chunk->prev;
    
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_free: checking for merge opportunities - next=%p, prev=%p", 
                 (void*)next, (void*)prev);
    }
    
    // Prova a fondere con il chunk successivo se libero e nello stesso segmento
    if (next && next->is_free && 
        next->start_segment == chunk->start_segment + chunk->num_segments - 1) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_free: merging with next chunk (size=%zu)", next->size);
        }
        
        // Rimuovi next dalla free list
        size_t next_bucket = log2(next->size);
        if (next_bucket < memory->num_free_buckets) {
            MemoryChunk** curr = &memory->free_chunks[next_bucket];
            while (*curr && *curr != next) curr = &(*curr)->next;
            if (*curr) *curr = (*curr)->next;
            
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_free: removed next chunk from bucket %zu", next_bucket);
            }
        }
        
        // Aggiorna dimensioni e segmenti
        chunk->size += next->size;
        chunk->num_segments += next->num_segments;
        
        // Ridimensiona e aggiorna array segment_sizes
        size_t* new_sizes = m3_Def_Realloc(chunk->segment_sizes, 
                                          chunk->num_segments * sizeof(size_t));
        if (new_sizes) {
            memcpy(new_sizes + chunk->num_segments - next->num_segments,
                  next->segment_sizes,
                  next->num_segments * sizeof(size_t));
            chunk->segment_sizes = new_sizes;
            
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_free: updated segment_sizes array after next merge");
            }
        } else {
            ESP_LOGE("WASM3", "m3_free: failed to reallocate segment_sizes array for next merge");
        }
        
        chunk->next = next->next;
        if (next->next) next->next->prev = chunk;
        
        m3_Def_Free(next->segment_sizes);
    }
    
    // Prova a fondere con il chunk precedente se libero
    if (prev && prev->is_free && 
        prev->start_segment + prev->num_segments == chunk->start_segment) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_free: merging with previous chunk (size=%zu)", prev->size);
        }
        
        // Rimuovi prev dalla free list
        size_t prev_bucket = log2(prev->size);
        if (prev_bucket < memory->num_free_buckets) {
            MemoryChunk** curr = &memory->free_chunks[prev_bucket];
            while (*curr && *curr != prev) curr = &(*curr)->next;
            if (*curr) *curr = (*curr)->next;
            
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_free: removed prev chunk from bucket %zu", prev_bucket);
            }
        }
        
        // Aggiorna dimensioni e segmenti
        prev->size += chunk->size;
        size_t old_num_segments = prev->num_segments;
        prev->num_segments += chunk->num_segments;
        
        // Ridimensiona e aggiorna array segment_sizes
        size_t* new_sizes = m3_Def_Realloc(prev->segment_sizes, 
                                          prev->num_segments * sizeof(size_t));
        if (new_sizes) {
            memcpy(new_sizes + old_num_segments,
                  chunk->segment_sizes,
                  chunk->num_segments * sizeof(size_t));
            prev->segment_sizes = new_sizes;
            
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_free: updated segment_sizes array after prev merge");
            }
        } else {
            ESP_LOGE("WASM3", "m3_free: failed to reallocate segment_sizes array for prev merge");
        }
        
        prev->next = chunk->next;
        if (chunk->next) chunk->next->prev = prev;
        
        // Usa prev come chunk principale
        chunk = prev;
        m3_Def_Free(chunk->segment_sizes);
    }
    
    // Marca il chunk come libero e aggiungilo alla free list appropriata
    chunk->is_free = true;
    size_t bucket = log2(chunk->size);
    if (bucket < memory->num_free_buckets) {
        chunk->next = memory->free_chunks[bucket];
        memory->free_chunks[bucket] = chunk;
        
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_free: added chunk to free bucket %zu", bucket);
        }
    } else {
        ESP_LOGE("WASM3", "m3_free: chunk size %zu too large for free buckets", chunk->size);
    }

    // Free empty segments
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_free: collecting empty segments");
    }
    m3_collect_empty_segments(memory);
}

void* m3_realloc(M3Memory* memory, void* offset_ptr, size_t new_size) {
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_realloc: entering with ptr=%p, new_size=%zu", offset_ptr, new_size);
    }
    
    if (!memory) {
        ESP_LOGE("WASM3", "m3_realloc: invalid memory pointer");
        return NULL;
    }
    
    if (!offset_ptr) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_realloc: null ptr, forwarding to m3_malloc");
        }
        return m3_malloc(memory, new_size);
    }
    
    if (new_size == 0) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_realloc: zero size, freeing memory");
        }
        m3_free(memory, offset_ptr);
        return NULL;
    }
    
    // Get current chunk
    u32 offset = (u32)offset_ptr;
    size_t segment_index = (offset - sizeof(MemoryChunk)) / memory->segment_size;
    if (segment_index >= memory->num_segments) {
        ESP_LOGE("WASM3", "m3_realloc: segment index %zu out of bounds", segment_index);
        return NULL;
    }
    
    MemorySegment* seg = memory->segments[segment_index];
    if (!seg || !seg->data) {
        ESP_LOGE("WASM3", "m3_realloc: invalid segment at index %zu", segment_index);
        return NULL;
    }
    
    size_t intra_segment_offset = (offset - sizeof(MemoryChunk)) % memory->segment_size;
    MemoryChunk* old_chunk = (MemoryChunk*)((char*)seg->data + intra_segment_offset - sizeof(MemoryChunk));
    
    // Verifica validità del chunk
    if (old_chunk->start_segment != segment_index || !old_chunk->segment_sizes) {
        ESP_LOGE("WASM3", "m3_realloc: invalid chunk metadata");
        return NULL;
    }
    
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_realloc: found chunk at segment %zu, current size=%zu", 
                 segment_index, old_chunk->size);
    }
    
    // Calcola nuova dimensione totale allineata
    size_t total_new_size = (new_size + sizeof(MemoryChunk) + (WASM_CHUNK_SIZE-1)) & 
                           ~(WASM_CHUNK_SIZE-1);
    
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_realloc: aligned total_new_size=%zu", total_new_size);
    }
    
    // Se la nuova dimensione è minore o uguale, possiamo ridurre il chunk
    if (total_new_size <= old_chunk->size) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_realloc: shrinking chunk");
        }
        
        // Per chunk multi-segmento, potremmo dover rilasciare alcuni segmenti
        if (old_chunk->num_segments > 1) {
            size_t new_segments_needed = (total_new_size + memory->segment_size - 1) / memory->segment_size;
            
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_realloc: multi-segment chunk needs %zu segments (current: %zu)", 
                         new_segments_needed, old_chunk->num_segments);
            }
            
            if (new_segments_needed < old_chunk->num_segments) {
                // Rimuovi il chunk dalle liste dei segmenti non più necessari
                for (size_t i = new_segments_needed; i < old_chunk->num_segments; i++) {
                    size_t curr_segment_index = old_chunk->start_segment + i;
                    if (curr_segment_index >= memory->num_segments) break;
                    
                    MemorySegment* curr_seg = memory->segments[curr_segment_index];
                    if (!curr_seg) continue;
                    
                    if (curr_seg->first_chunk == old_chunk) {
                        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                            ESP_LOGI("WASM3", "m3_realloc: removing chunk from segment %zu", 
                                     curr_segment_index);
                        }
                        curr_seg->first_chunk = old_chunk->next;
                    }
                }
                
                // Aggiorna array segment_sizes
                size_t* new_sizes = m3_Def_Realloc(old_chunk->segment_sizes, 
                                                  new_segments_needed * sizeof(size_t));
                if (new_sizes) {
                    old_chunk->segment_sizes = new_sizes;
                    old_chunk->num_segments = new_segments_needed;
                    
                    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                        ESP_LOGI("WASM3", "m3_realloc: updated segment_sizes array");
                    }
                } else {
                    ESP_LOGE("WASM3", "m3_realloc: failed to reallocate segment_sizes array");
                }
            }
        }
        
        old_chunk->size = total_new_size;
        return offset_ptr;
    }
    
    // Prova a espandere in-place
    bool can_expand = false;
    
    if (old_chunk->num_segments > 0) {
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_realloc: attempting in-place expansion");
        }
        
        // Verifica se ci sono segmenti liberi adiacenti
        size_t last_segment = old_chunk->start_segment + old_chunk->num_segments - 1;
        size_t additional_segments_needed = (total_new_size - old_chunk->size + memory->segment_size - 1) 
                                          / memory->segment_size;
        
        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_realloc: need %zu additional segments", additional_segments_needed);
        }
        
        bool segments_available = true;
        for (size_t i = 1; i <= additional_segments_needed; i++) {
            size_t check_segment = last_segment + i;
            if (check_segment >= memory->num_segments || 
                (memory->segments[check_segment] && memory->segments[check_segment]->first_chunk)) {
                segments_available = false;
                if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                    ESP_LOGI("WASM3", "m3_realloc: segment %zu not available", check_segment);
                }
                break;
            }
        }
        
        if (segments_available) {
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_realloc: adjacent segments are available for expansion");
            }
            
            // Espandi il chunk nei segmenti adiacenti
            size_t new_num_segments = old_chunk->num_segments + additional_segments_needed;
            size_t* new_sizes = m3_Def_Realloc(old_chunk->segment_sizes, 
                                              new_num_segments * sizeof(size_t));
            if (new_sizes) {
                old_chunk->segment_sizes = new_sizes;
                
                // Aggiorna le dimensioni dei segmenti
                size_t remaining_size = total_new_size - old_chunk->size;
                for (size_t i = old_chunk->num_segments; i < new_num_segments; i++) {
                    size_t segment_size = MIN(remaining_size, memory->segment_size);
                    old_chunk->segment_sizes[i] = segment_size;
                    remaining_size -= segment_size;
                    
                    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                        ESP_LOGI("WASM3", "m3_realloc: allocated %zu bytes in new segment %zu", 
                                segment_size, old_chunk->start_segment + i);
                    }
                    
                    // Inizializza il nuovo segmento se necessario
                    size_t curr_segment_index = old_chunk->start_segment + i;
                    if (!memory->segments[curr_segment_index]->data) {
                        if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                            ESP_LOGI("WASM3", "m3_realloc: initializing new segment %zu", 
                                    curr_segment_index);
                        }
                        if (InitSegment(memory, memory->segments[curr_segment_index], true) != NULL) {
                            ESP_LOGE("WASM3", "m3_realloc: failed to initialize segment %zu", 
                                    curr_segment_index);
                            break;
                        }
                    }
                    memory->segments[curr_segment_index]->first_chunk = old_chunk;
                }
                
                old_chunk->num_segments = new_num_segments;
                old_chunk->size = total_new_size;
                can_expand = true;
                
                if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                    ESP_LOGI("WASM3", "m3_realloc: in-place expansion successful");
                }
            } else {
                ESP_LOGE("WASM3", "m3_realloc: failed to reallocate segment_sizes array");
            }
        } else {
            if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "m3_realloc: in-place expansion not possible");
            }
        }
    }
    
    if (can_expand) {
        return offset_ptr;
    }
    
    // Se non possiamo espandere in-place, alloca nuovo chunk e copia i dati
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_realloc: allocating new chunk and copying data");
    }
    
    void* new_ptr = m3_malloc(memory, new_size);
    if (!new_ptr) {
        ESP_LOGE("WASM3", "m3_realloc: failed to allocate new chunk");
        return NULL;
    }
    
    // Copia i dati vecchi
    size_t old_data_size = old_chunk->size - sizeof(MemoryChunk);
    size_t copy_size = MIN(old_data_size, new_size);
    
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_realloc: copying %zu bytes from old chunk to new location", copy_size);
    }
    
    m3_memcpy(memory, new_ptr, offset_ptr, copy_size);
    
    // Libera il vecchio chunk
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_realloc: freeing old chunk");
    }
    m3_free(memory, offset_ptr);
    
    if(WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "m3_realloc: completed successfully");
    }
    return new_ptr;
}

///
///
///

const bool WASM_DEBUG_m3_memcpy = true;
M3Result m3_memcpy(M3Memory* memory, void* dest, const void* src, size_t n) {
    if(WASM_DEBUG_m3_memcpy) {
        ESP_LOGI("WASM3", "m3_memcpy called with dest=%p, src=%p, n=%zu", dest, src, n);
    }

    if (dest == NULL || src == NULL || !n) {       
        ESP_LOGE("WASM3", "m3_memcpy: invalid arguments - dest=%p, src=%p, n=%zu", dest, src, n);        
        return m3Err_malformedData;
    }

    // If memory is invalid, fall back to regular memcpy
    if(!IsValidMemory(memory)) {
        if(WASM_DEBUG_m3_memcpy) {
            ESP_LOGI("WASM3", "m3_memcpy: using direct memcpy for invalid memory");
            ESP_LOGW("WASM3", "m3_memcpy: dest=%p, src=%p", dest, src);
            LOG_FLUSH;
        }        

        memcpy(dest, src, n);
        return NULL;
    }

    // Check if dest and src are M3Memory offsets or absolute pointers
    bool dest_isSegmented = IsValidMemoryAccess(memory, (mos)dest, 0);
    bool src_isSegmented = IsValidMemoryAccess(memory, (mos)src, 0);

    if(WASM_DEBUG_m3_memcpy) {
        ESP_LOGI("WASM3", "m3_memcpy: dest_isSegmented=%d, src_isSegmented=%d", 
                 dest_isSegmented, src_isSegmented);
    }

    // Only use direct memcpy if BOTH pointers are absolute
    if (!dest_isSegmented && !src_isSegmented) {
        if(WASM_DEBUG_m3_memcpy) {
            ESP_LOGI("WASM3", "m3_memcpy: both pointers absolute, using direct memcpy");
        }
        memcpy(dest, src, n);
        return NULL;
    }

    // From here on, at least one pointer is segmented, so we need to handle segmented copy
    size_t remaining = n;
    size_t dest_offset = dest_isSegmented ? (u32)dest : 0;
    size_t src_offset = src_isSegmented ? (u32)src : 0;
    void* absolute_src = src_isSegmented ? NULL : src;
    void* absolute_dest = dest_isSegmented ? NULL : dest;

    while (remaining > 0) {
        // Handle destination
        size_t dest_segment = dest_isSegmented ? (dest_offset / memory->segment_size) : 0;
        size_t dest_segment_offset = dest_isSegmented ? (dest_offset % memory->segment_size) : 0;
        void* curr_dest = dest_isSegmented ? 
            ((char*)memory->segments[dest_segment]->data + dest_segment_offset) : 
            ((char*)absolute_dest + (n - remaining));

        // Handle source
        size_t src_segment = src_isSegmented ? (src_offset / memory->segment_size) : 0;
        size_t src_segment_offset = src_isSegmented ? (src_offset % memory->segment_size) : 0;
        void* curr_src = src_isSegmented ? 
            ((char*)memory->segments[src_segment]->data + src_segment_offset) : 
            ((char*)absolute_src + (n - remaining));

        // Calculate copy size for this iteration
        size_t dest_available = dest_isSegmented ? 
            (memory->segment_size - dest_segment_offset) : remaining;
        size_t src_available = src_isSegmented ? 
            (memory->segment_size - src_segment_offset) : remaining;
        size_t copy_size = MIN(MIN(dest_available, src_available), remaining);

        if(WASM_DEBUG_m3_memcpy) {
            ESP_LOGI("WASM3", "m3_memcpy: copying %zu bytes", copy_size);
        }

        memcpy(curr_dest, curr_src, copy_size);

        // Update counters
        remaining -= copy_size;
        if (dest_isSegmented) dest_offset += copy_size;
        if (src_isSegmented) src_offset += copy_size;
    }

    if(WASM_DEBUG_m3_memcpy) {
        ESP_LOGI("WASM3", "m3_memcpy: completed successfully");
    }
    return NULL;
}

////////////////////////////////////////////////////////////////

//todo: implement overflow handling
const bool WASM_DEBUG_m3_memset = true;
M3Result m3_memset(M3Memory* memory, void* ptr, int value, size_t n) {
    if(WASM_DEBUG_m3_memset) {
        ESP_LOGI("WASM3", "m3_memset called with ptr=%p, value=%d, n=%zu", ptr, value, n);
    }

    if (!ptr || !n) {
        ESP_LOGE("WASM3", "m3_memset: invalid arguments - ptr=%p, n=%zu", ptr, n);
        return m3Err_malformedData;
    }

    // If memory is invalid, fall back to regular memset
    if(!IsValidMemory(memory)) {
        if(WASM_DEBUG_m3_memset) {
            ESP_LOGI("WASM3", "m3_memset: using direct memset for invalid memory");
        }
        memset(ptr, value, n);
        return NULL;
    }

    // Check if ptr is a M3Memory offset
    bool is_segmented = IsValidMemoryAccess(memory, (mos)ptr, 0);
    
    if(WASM_DEBUG_m3_memset) {
        ESP_LOGI("WASM3", "m3_memset: ptr is %s", is_segmented ? "segmented" : "absolute");
    }

    // Get real pointer using resolve_pointer
    void* real_ptr = resolve_pointer(memory, ptr);
    
    if (real_ptr == ERROR_POINTER) {
        ESP_LOGE("WASM3", "m3_memset: failed to resolve pointer");
        return m3Err_malformedData;
    }

    if (!is_segmented) {
        // For absolute pointers, use direct memset
        if(WASM_DEBUG_m3_memset) {
            ESP_LOGI("WASM3", "m3_memset: using direct memset for absolute pointer");
        }
        memset(real_ptr, value, n);
        return NULL;
    }

    // Handle segmented memory
    size_t remaining = n;
    size_t current_offset = (u32)ptr;

    while (remaining > 0) {
        // Calculate current segment
        size_t segment_index = current_offset / memory->segment_size;
        
        if (segment_index >= memory->num_segments) {
            ESP_LOGE("WASM3", "m3_memset: segment index %zu out of bounds", segment_index);
            return m3Err_malformedData;
        }

        MemorySegment* segment = memory->segments[segment_index];
        if (!segment || !segment->data) {
            if(WASM_DEBUG_m3_memset) {
                ESP_LOGI("WASM3", "m3_memset: initializing segment %zu", segment_index);
            }
            if (InitSegment(memory, segment, true) != NULL) {
                ESP_LOGE("WASM3", "m3_memset: failed to initialize segment %zu", segment_index);
                return m3Err_malformedData;
            }
        }

        // Calculate how much we can set in this segment
        size_t segment_offset = current_offset % memory->segment_size;
        size_t available = memory->segment_size - segment_offset;
        size_t to_set = MIN(remaining, available);

        if(WASM_DEBUG_m3_memset) {
            ESP_LOGI("WASM3", "m3_memset: setting %zu bytes in segment %zu", 
                     to_set, segment_index);
        }

        // Perform the memset for this segment
        void* segment_ptr = (char*)segment->data + segment_offset;
        memset(segment_ptr, value, to_set);

        // Update counters
        remaining -= to_set;
        current_offset += to_set;

        if(WASM_DEBUG_m3_memset) {
            ESP_LOGI("WASM3", "m3_memset: remaining=%zu bytes", remaining);
        }
    }

    if(WASM_DEBUG_m3_memset) {
        ESP_LOGI("WASM3", "m3_memset: completed successfully");
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* m3SegmentedMemAccess(IM3Memory memory, void* offset, size_t size) {
    return resolve_pointer(memory, (void*)offset);
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//=================== GARBAGE COLLECTOR =============================///
////////////////////////////////////////////////////////////////////////

// Prototipi di funzioni helper
static bool is_segment_empty(IM3Memory memory, MemorySegment* segment);
static void deallocate_segment_data(IM3Memory memory, MemorySegment* segment);

void m3_collect_empty_segments(M3Memory* memory) {
    if (!memory || memory->firm != INIT_FIRM) {
        ESP_LOGW("WASM3", "m3_collect_empty_segments: invalid memory");
        return;
    }

    if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "Starting empty segments collection");
    }

    size_t freed_segments = 0;
    size_t freed_bytes = 0;

    // Mantieni almeno un segmento (il primo)
    for (size_t i = 1; i < memory->num_segments; i++) {
        MemorySegment* segment = memory->segments[i];
        
        if (!segment || !segment->data) {
            continue;
        }

        if (is_segment_empty(memory, segment)) {
            if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
                ESP_LOGI("WASM3", "Freeing empty segment %zu", i);
            }

            // Dealloca i dati del segmento
            deallocate_segment_data(memory, segment);
            freed_segments++;
            freed_bytes += segment->size;
            
            // Reset dei campi del segmento ma mantieni la struttura
            segment->data = NULL;
            segment->is_allocated = false;
            segment->size = 0;
            segment->first_chunk = NULL;
        }
    }

    if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "Garbage collection completed: freed %zu segments (%zu bytes)", 
                 freed_segments, freed_bytes);
        
        // Log dello stato della memoria dopo la GC
        size_t total_allocated = 0;
        size_t empty_segments = 0;
        for (size_t i = 0; i < memory->num_segments; i++) {
            if (memory->segments[i] && memory->segments[i]->data) {
                total_allocated += memory->segments[i]->size;
            } else {
                empty_segments++;
            }
        }
        ESP_LOGI("WASM3", "Memory status after GC: %zu empty segments, %zu bytes still allocated",
                 empty_segments, total_allocated);
    }

    // Aggiorna il contatore della memoria totale allocata
    memory->total_allocated_size -= freed_bytes;
}

static bool is_segment_empty(IM3Memory memory, MemorySegment* segment) {
    if (!segment || !segment->data) {
        return true;
    }

    // Verifica se ci sono chunk attivi
    MemoryChunk* current = segment->first_chunk;
    while (current) {
        if (!current->is_free) {
            // Trovato un chunk attivo
            return false;
        }
        current = current->next;
    }

    // Verifica che nessun chunk multi-segmento di altri segmenti si estenda in questo
    MemoryChunk** free_chunks = memory->free_chunks;
    size_t num_buckets = memory->num_free_buckets;

    for (size_t i = 0; i < num_buckets; i++) {
        MemoryChunk* chunk = free_chunks[i];
        while (chunk) {
            if (chunk->num_segments > 1) {
                // Calcola il range di segmenti occupati da questo chunk
                size_t end_segment = chunk->start_segment + chunk->num_segments - 1;
                
                // Se questo segmento cade nel range del chunk multi-segmento
                if (segment->index >= chunk->start_segment && 
                    segment->index <= end_segment) {
                    return false;
                }
            }
            chunk = chunk->next;
        }
    }

    return true;
}

static void deallocate_segment_data(IM3Memory memory, MemorySegment* segment) {
    if (!segment || !segment->data) {
        return;
    }

    // Libera tutti i segment_sizes arrays dei chunk nel segmento
    MemoryChunk* current = segment->first_chunk;
    while (current) {
        if (current->segment_sizes) {
            m3_Def_Free(current->segment_sizes);
            current->segment_sizes = NULL;
        }
        current = current->next;
    }

    // Libera i dati del segmento
    m3_Def_Free(segment->data);
    segment->data = NULL;
    memory->total_allocated_size -= segment->size;
    segment->size = 0;
    segment->is_allocated = false;
    segment->first_chunk = NULL;

    if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
        ESP_LOGI("WASM3", "Deallocated segment data at index %zu", segment->index);
    }
}

// Helper per verificare se un segmento è parte di un chunk multi-segmento attivo
static bool is_part_of_active_multisegment(M3Memory* memory, size_t segment_index) {
    for (size_t i = 0; i < memory->num_segments; i++) {
        MemorySegment* seg = memory->segments[i];
        if (!seg || !seg->data) continue;

        MemoryChunk* chunk = seg->first_chunk;
        while (chunk) {
            if (!chunk->is_free && chunk->num_segments > 1) {
                size_t end_segment = chunk->start_segment + chunk->num_segments - 1;
                if (segment_index >= chunk->start_segment && 
                    segment_index <= end_segment) {
                    return true;
                }
            }
            chunk = chunk->next;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
///// Memory chunks

ChunkInfo get_chunk_info(M3Memory* memory, void* ptr) {
    ChunkInfo result = { NULL, 0 };
    if (!memory || !ptr) return result;
    
    // Prima verifica se il puntatore è un offset
    bool is_offset = ((uintptr_t)ptr < memory->total_size);
    size_t segment_index;
    size_t intra_offset;
    
    if (is_offset) {
        // Calcola segmento e offset dall'offset fornito
        segment_index = (uintptr_t)ptr / memory->segment_size;
        intra_offset = (uintptr_t)ptr % memory->segment_size;
    } else {
        // Cerca in quale segmento si trova il puntatore diretto
        bool found = false;
        for (size_t i = 0; i < memory->num_segments; i++) {
            MemorySegment* seg = memory->segments[i];
            if (!seg || !seg->data) continue;
            
            if (ptr >= seg->data && ptr < (char*)seg->data + seg->size) {
                segment_index = i;
                intra_offset = (char*)ptr - (char*)seg->data;
                found = true;
                break;
            }
        }
        if (!found) return result;
    }
    
    // Verifica validità del segmento
    if (segment_index >= memory->num_segments) return result;
    MemorySegment* seg = memory->segments[segment_index];
    if (!seg || !seg->data) return result;
    
    // Cerca il chunk che contiene questo puntatore
    MemoryChunk* current = seg->first_chunk;
    while (current) {
        // Calcola l'area di memoria occupata da questo chunk
        void* chunk_start = (void*)current;
        void* chunk_end = (void*)((char*)chunk_start + current->size);
        
        // Per chunk multi-segmento
        if (current->num_segments > 1) {
            size_t total_size = 0;
            for (size_t i = 0; i < current->num_segments; i++) {
                total_size += current->segment_sizes[i];
            }
            chunk_end = (void*)((char*)chunk_start + total_size);
        }
        
        // Verifica se il puntatore cade in questo chunk
        if ((is_offset && intra_offset >= (uintptr_t)chunk_start - (uintptr_t)seg->data && 
             intra_offset < (uintptr_t)chunk_end - (uintptr_t)seg->data) ||
            (!is_offset && ptr >= chunk_start && ptr < chunk_end)) {
            
            result.chunk = current;
            // Calcola l'offset base del chunk
            result.base_offset = (current->start_segment * memory->segment_size) + 
                               ((char*)current - (char*)memory->segments[current->start_segment]->data) +
                               sizeof(MemoryChunk);
            break;
        }
        
        current = current->next;
    }
    
    if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC && result.chunk) {
        ESP_LOGI("WASM3", "Found chunk: segment=%d, size=%zu, base_offset=%u", 
                 result.chunk->start_segment, result.chunk->size, result.base_offset);
    }
    
    return result;
}

// Funzione helper per ottenere solo il chunk
MemoryChunk* get_chunk(M3Memory* memory, void* ptr) {
    return get_chunk_info(memory, ptr).chunk;
}

// Funzione helper per ottenere solo l'offset base
u32 get_chunk_base_offset(M3Memory* memory, void* ptr) {
    return get_chunk_info(memory, ptr).base_offset;
}