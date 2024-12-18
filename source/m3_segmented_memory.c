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
void* get_segment_pointer(IM3Memory memory, u32 offset) {
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
    if (!seg->data && InitSegment(memory, seg, true) != NULL) {
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

// Memory initialization and growth
M3Result InitSegment(M3Memory* memory, MemorySegment* seg, bool initData) {
    if (!memory) return m3Err_nullMemory;
    
    if (!seg) {
        seg = m3_Def_Malloc(sizeof(MemorySegment));
        if (!seg) return m3Err_mallocFailed;
    }
    
    seg->firm = INIT_FIRM;
    
    if (initData && !seg->data) {
        seg->data = m3_Def_Malloc(memory->segment_size);
        if (!seg->data) return m3Err_mallocFailed;
        
        memset(seg->data, 0, memory->segment_size);  // Zero initialize
        
        seg->is_allocated = true;
        seg->size = memory->segment_size;
        seg->first_chunk = NULL;
        memory->total_allocated_size += memory->segment_size;
    }
    
    return m3Err_none;
}

const bool WASM_ADD_SEGMENTS_MALLOC_SEGMENT = false;
M3Result AddSegments(M3Memory* memory, size_t additional_segments) {
    if (!memory || memory->firm != INIT_FIRM) return m3Err_nullMemory;
    
    size_t new_num_segments = memory->num_segments + additional_segments;
    size_t new_size = new_num_segments * sizeof(MemorySegment*);
    
    MemorySegment** new_segments = m3_Def_Realloc(memory->segments, new_size);
    if (!new_segments) return m3Err_mallocFailed;
    
    memory->segments = new_segments;
    
    // Initialize new segments
    for (size_t i = memory->num_segments; i < new_num_segments; i++) {
        if(WASM_ADD_SEGMENTS_MALLOC_SEGMENT){
            memory->segments[i] = m3_Def_Malloc(sizeof(MemorySegment));
            if (!memory->segments[i]) {
                // Rollback on failure
                for (size_t j = memory->num_segments; j < i; j++) {
                    m3_Def_Free(memory->segments[j]);
                }
                return m3Err_mallocFailed;
            }
        }
        
        M3Result result = InitSegment(memory, memory->segments[i], false);
        if (result) {
            // Cleanup on failure
            for (size_t j = memory->num_segments; j <= i; j++) {
                m3_Def_Free(memory->segments[j]);
            }
            return result;
        }
    }
    
    memory->num_segments = new_num_segments;
    memory->total_size = memory->segment_size * new_num_segments;
    
    return m3Err_none;
}

const bool WASM_DEBUG_INIT_MEMORY = true;
IM3Memory m3_InitMemory(IM3Memory memory) {
    if (!memory) return NULL;
    
    memory->firm = INIT_FIRM;
    memory->segments = NULL;
    memory->num_segments = 0;
    memory->total_size = 0;
    memory->total_allocated_size = 0;
    memory->segment_size = WASM_SEGMENT_SIZE;
    memory->maxPages = M3Memory_MaxPages;
    memory->total_requested_size = 0;
    
    // Initialize free chunks management
    memory->num_free_buckets = 32;
    memory->free_chunks = m3_Def_Malloc(memory->num_free_buckets * sizeof(MemoryChunk*));
    if (!memory->free_chunks) return NULL;
    
    // Add initial segments
    M3Result result = AddSegments(memory, WASM_INIT_SEGMENTS);
    if (result != m3Err_none) {
        free(memory->free_chunks);
        return NULL;
    }

    if(WASM_DEBUG_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: trying to allocate memory");
    void* ptr1 = m3_malloc(memory, 1);
    void* ptr2 = m3_malloc(memory, 1);
    PRINT_PTR1(ptr1);
    PRINT_PTR2(ptr2);
    
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
bool IsValidMemoryAccess(IM3Memory memory, mos offset, size_t size) {
    if (!memory || !memory->segments) return false;
    if (offset + size > memory->total_size) return false;
    
    size_t start_segment = offset / memory->segment_size;
    size_t end_segment = (offset + size - 1) / memory->segment_size;
    
    // Verify all needed segments exist and are initialized
    for (size_t i = start_segment; i <= end_segment; i++) {
        if (i >= memory->num_segments) return false;
        
        MemorySegment* seg = memory->segments[i];
        if (!seg || !seg->data) return false;
        
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
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void m3_free(M3Memory* memory, void* offset_ptr) {
    if (!memory || !offset_ptr) return;
    
    u32 offset = (u32)offset_ptr;
    void* real_ptr = get_segment_pointer(memory, offset - sizeof(MemoryChunk));
    if (real_ptr == ERROR_POINTER) return;
    
    MemoryChunk* chunk = (MemoryChunk*)real_ptr;
    if (!chunk) return;
    
    // Free multi-segment resources
    if (chunk->num_segments > 1) {
        m3_Def_Free(chunk->segment_sizes);
    }
    
    // Try to merge with adjacent free chunks
    MemoryChunk* next = chunk->next;
    MemoryChunk* prev = chunk->prev;
    
    if (next && next->is_free) {
        // Merge with next chunk
        chunk->size += next->size;
        chunk->next = next->next;
        if (next->next) next->next->prev = chunk;
        
        if (next->num_segments > 1) {
            m3_Def_Free(next->segment_sizes);
        }
    }
    
    if (prev && prev->is_free) {
        // Merge with previous chunk
        prev->size += chunk->size;
        prev->next = chunk->next;
        if (chunk->next) chunk->next->prev = prev;
        
        if (chunk->num_segments > 1) {
            m3_Def_Free(chunk->segment_sizes);
        }
        chunk = prev;  // Use the merged chunk for free list
    }
    
    // Add to free list
    chunk->is_free = true;
    size_t bucket = log2(chunk->size);
    if (bucket < memory->num_free_buckets) {
        chunk->next = memory->free_chunks[bucket];
        memory->free_chunks[bucket] = chunk;
    }
}

void m3_memcpy(M3Memory* memory, void* dest, const void* src, size_t n) {
    if (!memory || !dest || !src || !n) return;
    
    // Resolve source and destination pointers
    void* real_src = resolve_pointer(memory, (void*)src);
    void* real_dest = resolve_pointer(memory, dest);
    
    if (real_src == ERROR_POINTER || real_dest == ERROR_POINTER) {
        return;
    }
    
    // Find chunks if pointers are in segmented memory
    MemoryChunk* src_chunk = NULL;
    MemoryChunk* dest_chunk = NULL;
    
    for (size_t i = 0; i < memory->num_segments; i++) {
        MemorySegment* seg = memory->segments[i];
        if (!seg || !seg->data) continue;
        
        if (is_address_in_segment(seg, real_src)) {
            MemoryChunk* chunk = seg->first_chunk;
            while (chunk) {
                void* chunk_data = (void*)((char*)chunk + sizeof(MemoryChunk));
                if (real_src >= chunk_data && real_src < (void*)((char*)chunk_data + chunk->size)) {
                    src_chunk = chunk;
                    break;
                }
                chunk = chunk->next;
            }
        }
        
        if (is_address_in_segment(seg, real_dest)) {
            MemoryChunk* chunk = seg->first_chunk;
            while (chunk) {
                void* chunk_data = (void*)((char*)chunk + sizeof(MemoryChunk));
                if (real_dest >= chunk_data && real_dest < (void*)((char*)chunk_data + chunk->size)) {
                    dest_chunk = chunk;
                    break;
                }
                chunk = chunk->next;
            }
        }
    }
    
    // Handle copying
    if (!src_chunk && !dest_chunk) {
        // Simple case: neither source nor destination are in multi-segment chunks
        memcpy(real_dest, real_src, n);
    } else {
        // Complex case: at least one pointer is in a multi-segment chunk
        size_t remaining = n;
        char* curr_src = (char*)real_src;
        char* curr_dest = (char*)real_dest;
        
        while (remaining > 0) {
            size_t copy_size = remaining;
            
            // Calculate maximum contiguous copy size
            if (src_chunk && src_chunk->num_segments > 1) {
                size_t src_seg_idx = (curr_src - (char*)memory->segments[src_chunk->start_segment]->data) 
                                   / memory->segment_size;
                size_t src_seg_offset = (curr_src - (char*)memory->segments[src_chunk->start_segment]->data) 
                                      % memory->segment_size;
                copy_size = MIN(copy_size, src_chunk->segment_sizes[src_seg_idx] - src_seg_offset);
            }
            
            if (dest_chunk && dest_chunk->num_segments > 1) {
                size_t dest_seg_idx = (curr_dest - (char*)memory->segments[dest_chunk->start_segment]->data) 
                                    / memory->segment_size;
                size_t dest_seg_offset = (curr_dest - (char*)memory->segments[dest_chunk->start_segment]->data) 
                                       % memory->segment_size;
                copy_size = MIN(copy_size, dest_chunk->segment_sizes[dest_seg_idx] - dest_seg_offset);
            }
            
            // Perform the copy for this segment
            memcpy(curr_dest, curr_src, copy_size);
            
            curr_src += copy_size;
            curr_dest += copy_size;
            remaining -= copy_size;
            
             // Handle segment transitions
            if (remaining > 0) {
                if (src_chunk && src_chunk->num_segments > 1) {
                    size_t seg_idx = (curr_src - (char*)memory->segments[src_chunk->start_segment]->data) 
                                   / memory->segment_size;
                    if (seg_idx < src_chunk->num_segments - 1) {
                        curr_src = (char*)memory->segments[src_chunk->start_segment + seg_idx + 1]->data;
                    }
                }
                
                if (dest_chunk && dest_chunk->num_segments > 1) {
                    size_t seg_idx = (curr_dest - (char*)memory->segments[dest_chunk->start_segment]->data) 
                                   / memory->segment_size;
                    if (seg_idx < dest_chunk->num_segments - 1) {
                        curr_dest = (char*)memory->segments[dest_chunk->start_segment + seg_idx + 1]->data;
                    }
                }
            }
        }
    }
}

void* m3_realloc(M3Memory* memory, void* offset_ptr, size_t new_size) {
    if (!memory) return NULL;
    if (!offset_ptr) return m3_malloc(memory, new_size);
    if (new_size == 0) {
        m3_free(memory, offset_ptr);
        return NULL;
    }
    
    // Get current chunk
    u32 offset = (u32)offset_ptr;
    void* real_ptr = get_segment_pointer(memory, offset - sizeof(MemoryChunk));
    if (real_ptr == ERROR_POINTER) return NULL;
    
    MemoryChunk* old_chunk = (MemoryChunk*)real_ptr;
    size_t old_size = old_chunk->size - sizeof(MemoryChunk);
    size_t total_new_size = new_size + sizeof(MemoryChunk);

    // Check if we can expand in place
    bool can_expand_in_place = false;
    
    // Case 1: Single segment chunk with enough space in current segment
    if (old_chunk->num_segments == 1) {
        MemorySegment* seg = memory->segments[old_chunk->start_segment];
        size_t available_space = seg->size - ((char*)old_chunk - (char*)seg->data);
        
        if (!old_chunk->next && available_space >= total_new_size) {
            old_chunk->size = total_new_size;
            return offset_ptr;
        }
        
        // Check if next chunk is free and provides enough space
        if (old_chunk->next && old_chunk->next->is_free) {
            size_t combined_size = old_chunk->size + old_chunk->next->size;
            if (combined_size >= total_new_size) {
                // Remove next chunk from free list
                size_t bucket = log2(old_chunk->next->size);
                if (bucket < memory->num_free_buckets) {
                    MemoryChunk** curr = &memory->free_chunks[bucket];
                    while (*curr && *curr != old_chunk->next) {
                        curr = &(*curr)->next;
                    }
                    if (*curr) *curr = (*curr)->next;
                }
                
                // Merge chunks
                old_chunk->size = total_new_size;
                old_chunk->next = old_chunk->next->next;
                if (old_chunk->next) {
                    old_chunk->next->prev = old_chunk;
                }
                
                return offset_ptr;
            }
        }
    }
    
    // Case 2: Multi-segment chunk with possibility to expand
    if (old_chunk->num_segments > 1) {
        size_t last_segment = old_chunk->start_segment + old_chunk->num_segments - 1;
        MemorySegment* last_seg = memory->segments[last_segment];
        size_t used_in_last = old_chunk->segment_sizes[old_chunk->num_segments - 1];
        
        if (used_in_last < last_seg->size) {
            size_t additional_available = last_seg->size - used_in_last;
            size_t total_available = old_chunk->size + additional_available;
            
            if (total_available >= total_new_size) {
                old_chunk->size = total_new_size;
                old_chunk->segment_sizes[old_chunk->num_segments - 1] += (total_new_size - old_chunk->size);
                return offset_ptr;
            }
        }
        
        // Check if we can add more segments
        if (last_segment + 1 < memory->num_segments) {
            MemorySegment* next_seg = memory->segments[last_segment + 1];
            if (!next_seg->first_chunk) {
                // Can expand into next segment
                size_t* new_sizes = m3_Def_Realloc(old_chunk->segment_sizes, 
                                                  (old_chunk->num_segments + 1) * sizeof(size_t));
                if (new_sizes) {
                    old_chunk->segment_sizes = new_sizes;
                    old_chunk->num_segments++;
                    old_chunk->size = total_new_size;
                    old_chunk->segment_sizes[old_chunk->num_segments - 1] = 
                        total_new_size - (old_chunk->size - old_chunk->segment_sizes[old_chunk->num_segments - 2]);
                    return offset_ptr;
                }
            }
        }
    }
    
    // If we can't expand in place, allocate new chunk and copy data
    void* new_offset = m3_malloc(memory, new_size);
    if (!new_offset) return NULL;
    
    // Copy old data
    m3_memcpy(memory, new_offset, offset_ptr, MIN(old_size, new_size));
    
    // Free old chunk
    m3_free(memory, offset_ptr);
    
    return new_offset;
}

///
///
///

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

void* m3_malloc(M3Memory* memory, size_t size) {
    if (!memory || memory->firm != INIT_FIRM || size == 0) return NULL;
    
    if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) 
        ESP_LOGI("WASM3", "m3_malloc: entering with size=%zu", size);
    
    size_t total_size = size + sizeof(MemoryChunk);
    size_t bucket = log2(total_size);
    MemoryChunk* found_chunk = NULL;    
    
    // 1. Prima cerca nei chunk liberi
    if (bucket < memory->num_free_buckets) {
        MemoryChunk** curr = &memory->free_chunks[bucket];
        while (*curr) {
            if ((*curr)->size >= total_size) {
                found_chunk = *curr;
                *curr = found_chunk->next;
                if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) 
                    ESP_LOGI("WASM3", "m3_malloc: found free chunk of size=%zu", found_chunk->size);
                break;
            }
            curr = &(*curr)->next;
        }
    }
    
    // 2. Se non trova chunk liberi, cerca spazio alla fine dei segmenti esistenti
    if (!found_chunk) {
        if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) 
            ESP_LOGI("WASM3", "m3_malloc: searching for space in existing segments");
            
        for (size_t i = 0; i < memory->num_segments; i++) {
            MemorySegment* seg = memory->segments[i];
            if (!seg || !seg->data) {
                if (InitSegment(memory, seg, true) != NULL)
                    continue;
            }
            
            // Calcola spazio utilizzato nel segmento
            MemoryChunk* last_chunk = seg->first_chunk;
            size_t used_space = 0;
            
            while (last_chunk && last_chunk->next) {
                used_space += last_chunk->size;
                last_chunk = last_chunk->next;
            }
            if (last_chunk) {
                used_space += last_chunk->size;
            }
            
            // Se c'è spazio sufficiente dopo l'ultimo chunk
            if (used_space + total_size <= seg->size) {
                found_chunk = (MemoryChunk*)((char*)seg->data + used_space);
                found_chunk->size = total_size;
                found_chunk->is_free = false;
                found_chunk->next = NULL;
                found_chunk->prev = last_chunk;
                found_chunk->num_segments = 1;
                found_chunk->start_segment = i;
                found_chunk->segment_sizes = m3_Def_Malloc(sizeof(size_t));
                if (!found_chunk->segment_sizes) return NULL;
                found_chunk->segment_sizes[0] = total_size;
                
                if (last_chunk) {
                    last_chunk->next = found_chunk;
                } else {
                    seg->first_chunk = found_chunk;
                }
                
                if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) 
                    ESP_LOGI("WASM3", "m3_malloc: allocated in existing segment %zu", i);
                break;
            }
        }
    }
    
    // 3. Se non trova spazio nei segmenti esistenti, alloca nuovi segmenti
    if (!found_chunk) {
        if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) 
            ESP_LOGI("WASM3", "m3_malloc: allocating new segments");
        
        // Calcola quanti segmenti servono
        size_t segments_needed = (total_size + memory->segment_size - 1) / memory->segment_size;
        
        // Aggiungi i segmenti necessari
        M3Result result = AddSegments(memory, segments_needed);
        if (result != NULL) {
            ESP_LOGE("WASM3", "m3_malloc: failed to add new segments");
            return NULL;
        }
        
        // Usa il primo nuovo segmento
        size_t new_segment_index = memory->num_segments - segments_needed;
        MemorySegment* new_seg = memory->segments[new_segment_index];
        
        if (!new_seg->data) {
            if (InitSegment(memory, new_seg, true) != NULL) {
                ESP_LOGE("WASM3", "m3_malloc: failed to initialize new segment");
                return NULL;
            }
        }
        
        found_chunk = (MemoryChunk*)new_seg->data;
        found_chunk->size = total_size;
        found_chunk->is_free = false;
        found_chunk->next = NULL;
        found_chunk->prev = NULL;
        found_chunk->num_segments = segments_needed;
        found_chunk->start_segment = new_segment_index;
        found_chunk->segment_sizes = m3_Def_Malloc(sizeof(size_t) * segments_needed);
        if (!found_chunk->segment_sizes) return NULL;
        
        // Distribuisci la size tra i segmenti
        size_t remaining_size = total_size;
        for (size_t i = 0; i < segments_needed; i++) {
            size_t available = memory->segment_size;
            if (i == 0) available -= sizeof(MemoryChunk);
            found_chunk->segment_sizes[i] = (remaining_size > available) ? available : remaining_size;
            remaining_size -= found_chunk->segment_sizes[i];
        }
        
        new_seg->first_chunk = found_chunk;
    }
    
    if (found_chunk) {
        // Calcola l'offset relativo al primo segmento
        u32 offset = (found_chunk->start_segment * memory->segment_size) + sizeof(MemoryChunk);
        
        if (WASM_DEBUG_SEGMENTED_MEMORY_ALLOC) {
            ESP_LOGI("WASM3", "m3_malloc: returning offset=%u", offset);
        }
        
        return (void*)offset;
    }
    
    ESP_LOGE("WASM3", "m3_malloc: allocation failed");
    return NULL;
}

void* m3SegmentedMemAccess(IM3Memory memory, void* offset, size_t size) {
    return resolve_pointer(memory, (void*)offset);
}