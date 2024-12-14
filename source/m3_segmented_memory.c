#include "m3_segmented_memory.h"
#include "esp_log.h"

#include "m3_pointers.h"

#define WASM_DEBUG_SEGMENTED_MEM_MAN 1


// Funzioni helper migliorate
static MemoryChunk* get_chunk_header(void* ptr) {
    return (MemoryChunk*)((char*)ptr - sizeof(MemoryChunk));
}

static void* get_chunk_data(MemoryChunk* chunk) {
    return (void*)((char*)chunk + sizeof(MemoryChunk));
}

static size_t align_size(size_t size) {
    return ((size + sizeof(MemoryChunk) + (WASM_CHUNK_SIZE-1)) & ~(WASM_CHUNK_SIZE-1));
}


IM3Memory m3_NewMemory(){
    IM3Memory memory = m3_Def_AllocStruct (M3Memory);

    m3_InitMemory(memory);

    return memory;
}

//todo: deprecate it
bool allocate_segment_data(M3Memory* memory, size_t segment_index) {
    if (!memory || segment_index >= memory->num_segments) {
        ESP_LOGI("WASM3", "allocate_segment_data: memory null or segment_index > num_segments (%d > %d)", segment_index, memory->num_segments);
        return false;
    }

    if (WASM_DEBUG_SEGMENTED_MEM_MAN) {
        ESP_LOGI("WASM3", "allocate_segment_data: Allocating segment %zu of size %zu", segment_index, memory->segment_size);
        ESP_LOGI("WASM3", "allocate_segment_data: flush");
    }

    MemorySegment* segment = memory->segments[segment_index];

    // Se il segmento è già allocato, restituisci true
    if (segment->is_allocated) {
        return true;
    }

    // Prima prova ad allocare in SPIRAM se abilitata
    void* ptr = default_malloc(memory->segment_size);

    if (ptr) {
        // Inizializza il segmento con zeri
        memset(ptr, 0, memory->segment_size);
        
        segment->data = ptr;
        segment->is_allocated = true;
        
        if (WASM_DEBUG_SEGMENTED_MEM_MAN) {
            ESP_LOGI("WASM3", "Segment %zu successfully allocated", segment_index);
        }
        
        return true;
    }

    return false;
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
    
    M3Result result = AddSegment(memory, additional_segments);
    if (result != NULL) {
        return result;
    }
    
    memory->total_size = new_total;

    m3_malloc(memory, 1); // Allocate memory
    
    return NULL;
}

// Funzione per aggiungere un nuovo segmento
const bool WASM_DEBUG_ADD_SEGMENT = false;
M3Result AddSegment(M3Memory* memory, size_t set_num_segments) {
    if(memory->segment_size == 0){
        ESP_LOGE("WASM3", "AddSegment: memory->segment_size is zero");
        backtrace();
        return m3Err_nullMemory;
    }

    size_t original_num_segments = memory->num_segments;

    size_t new_segments = set_num_segments;
    if(new_segments == 0){
        new_segments = memory->num_segments + 1;
    }

    size_t new_size = (new_segments + 1) * sizeof(MemorySegment*);
    
    if(new_segments > memory->num_segments){
        if(WASM_DEBUG_ADD_SEGMENT) ESP_LOGI("WASM3", "Adding segments (%d)", new_segments);
        MemorySegment** new_segments = m3_Def_Realloc(memory->segments, new_size);
        if (!new_segments) {
            ESP_LOGE("WASM3", "AddSegment: failed to allocate new segments");
            memory->num_segments = original_num_segments;
            return m3Err_mallocFailed;
        }

        memory->segments = new_segments;
    }

    if(WASM_DEBUG_ADD_SEGMENT)
        ESP_LOGI("WASM3", "AddSegment: num_segments = %d, new_num_segments: %d", memory->num_segments, new_segments);

    size_t new_idx = memory->num_segments;
    for(; new_idx < new_segments; new_idx++){   
        if(WASM_DEBUG_ADD_SEGMENT){
            ESP_LOGI("WASM3", "AddSegment: memory->segments[%d]=%p", new_idx, memory->segments[new_idx]);
        }

        // Allocare la nuova struttura MemorySegment
        memory->segments[new_idx] = m3_Def_Malloc(sizeof(MemorySegment));
        if (!memory->segments[new_idx]) {
            ESP_LOGE("WASM3", "AddSegment: failed to allocate segment number %d", new_idx);
            goto backToOriginal;
        }                

        // Verifica che il puntatore sia valido prima di usarlo
        if (!ultra_safe_ptr_valid(memory->segments[new_idx])) {
            ESP_LOGE("WASM3", "AddSegment: Invalid pointer at memory->segments[%d]", new_idx);
            goto backToOriginal;
        }
        
        // Verifica che possiamo effettivamente accedere alla struttura
        MemorySegment* seg = memory->segments[new_idx];
        if(WASM_DEBUG_ADD_SEGMENT) ESP_LOGI("WASM3", "AddSegment: Can access segment structure");

        // Allocare i dati del segmento
        seg->data = m3_Def_Malloc(memory->segment_size);
        if (!seg->data) {  
            ESP_LOGE("WASM3", "AddSegment: can't allocate segment data");          
            goto backToOriginal;
        }
        
        seg->is_allocated = true;
        seg->size = memory->segment_size;
        memory->total_size += memory->segment_size;                   
    }

    memory->num_segments = new_segments;

    return NULL;     

    backToOriginal:

    for(; new_idx >= original_num_segments; new_idx--){
        MemorySegment* seg = memory->segments[new_idx];
        m3_Def_Free(seg);
    }

    return m3Err_mallocFailed;
}

const bool WASM_DEBUG_SEGMENTED_MEM_ACCESS = true;

const bool WASM_DEBUG_MEM_ACCESS = true;

void* resolve_pointer(IM3Memory memory, void* ptr) {
    ESP_LOGI("WASM3", "resolve_pointer start");
    
    // Verifiche base più dettagliate
    if (!memory) {
        ESP_LOGE("WASM3", "memory is NULL");
        goto returnOriginal;
    }
    
    ESP_LOGI("WASM3", "memory->num_segments: %d", memory->num_segments);
    
    if (!memory->segments) {
        ESP_LOGE("WASM3", "memory->segments is NULL");
        goto returnOriginal;
    }
    
    if (memory->num_segments == 0) {
        ESP_LOGE("WASM3", "memory->num_segments is 0");
        goto returnOriginal;
    }

    // Dobbiamo trovare il primo e l'ultimo segmento valido
    void* seg_start = NULL;
    void* seg_end = NULL;
    
    // Trova il primo segmento allocato
    for (size_t i = 0; i < memory->num_segments; i++) {
        if (!memory->segments[i]) {
            ESP_LOGE("WASM3", "memory->segments[%d] is NULL", i);
            goto returnOriginal;
        }
        
        if (memory->segments[i] && memory->segments[i]->data) {
            seg_start = memory->segments[i]->data;
            break;
        }
    }
    
    // Se non troviamo nemmeno un segmento valido, ritorna il puntatore originale
    if (!seg_start) {
        goto returnOriginal;
    }
    
    // Trova l'ultimo segmento allocato
    for (size_t i = memory->num_segments - 1; i >= 0; i--) {
        if (memory->segments[i] && memory->segments[i]->data) {
            seg_end = (uint8_t*)memory->segments[i]->data + memory->segment_size;
            break;
        }
    }

    // Se il puntatore è nel range della memoria segmentata
    if (ptr >= seg_start && ptr < seg_end) {
        // Calcola l'offset dall'inizio della memoria segmentata
        size_t offset = (uintptr_t)ptr - (uintptr_t)seg_start;
        
        // Trova il segmento corretto
        size_t segment_index = offset / memory->segment_size;
        size_t segment_offset = offset % memory->segment_size;
        
        // Verifica che il segmento esista ed sia allocato
        if (segment_index < memory->num_segments && 
            memory->segments[segment_index] && 
            memory->segments[segment_index]->data &&
            memory->segments[segment_index]->is_allocated) {
            
            uint8_t* res = (uint8_t*)memory->segments[segment_index]->data + segment_offset;

            if(WASM_DEBUG_MEM_ACCESS){ 
                ESP_LOGI("WASM3", "resolve_pointer: returning M3Memory pointer %p", res);
                ESP_LOGI("WASM3", "resolve_pointer: returning M3Memory pointer");
            }

            return res;
        }
    }
    
    returnOriginal:

    // Se non è nella memoria segmentata, ritorna il puntatore originale
    if(WASM_DEBUG_MEM_ACCESS){ 
        ESP_LOGI("WASM3", "resolve_pointer: returning original pointer %p", ptr);
        ESP_LOGI("WASM3", "resolve_pointer: returning M3Memory pointer");
    }

    if(is_ptr_valid(ptr)){
        ESP_LOGE("WASM3", "resolve_pointer: invalid pointer %p", ptr);
        return NULL;
    }

    return ptr;
}

const bool WASM_DEBUG_MEM_ACCESS_BACKTRACE = false;
void* m3SegmentedMemAccess(IM3Memory mem, void* ptr, size_t size) 
{
    u32 offset = (u32)ptr;

    if(WASM_DEBUG_MEM_ACCESS){
        ESP_LOGI("WASM3", "m3SegmentedMemAccess: requested memory %d", offset);
    }

    if(mem == NULL){
        ESP_LOGE("WASM3", "m3SegmentedMemAccess called with null memory pointer");     
        if(WASM_DEBUG_MEM_ACCESS_BACKTRACE)
            backtrace();
        return NULL;
    }

    if(WASM_DEBUG_SEGMENTED_MEM_ACCESS){ 
        ESP_LOGI("WASM3", "m3SegmentedMemAccess call");         
        //ESP_LOGI("WASM3", "m3SegmentedMemAccess: mem = %p", (void*)mem);  
    }

    return resolve_pointer(mem, ptr);

    // Verifica che l'accesso sia nei limiti della memoria totale
    if (mem->total_size > 0 && offset + size > mem->total_size){
        ESP_LOGE("WASM3", "m3SegmentedMemAccess: requested memory exceeds total size");
        return NULL;
    }    

    size_t segment_index = offset / mem->segment_size;
    size_t segment_offset = offset % mem->segment_size;
    
    // Verifica se stiamo accedendo attraverso più segmenti
    size_t end_segment = (offset + size - 1) / mem->segment_size;
    
    // Alloca tutti i segmenti necessari se non sono già allocati
    for (size_t i = segment_index; i <= end_segment; i++) {
        if (!mem->segments[i]->is_allocated) {
            if (!allocate_segment_data(mem, i)) {
                ESP_LOGE("WASM3", "Failed to allocate segment %zu on access", i);
                return NULL;
            }
            if(WASM_DEBUG_SEGMENTED_MEM_ACCESS) ESP_LOGI("WASM3", "Lazy allocated segment %zu on access", i);
        }
    }
    
    // Ora possiamo essere sicuri che il segmento è allocato
    return ((void*)mem->segments[segment_index]->data) + segment_offset;
}

bool IsValidMemoryAccess(IM3Memory memory, u64 offset, u32 size)
{
    return (offset + size) <= memory->total_size;
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

const bool WASM_DEBUG_SEGMENTED_MEMORY = false;


#define PRINT_PTR(ptr) ESP_LOGI("WASM3", "Pointer value: %p (unsigned: %u, signed: %d)", \
                               (void*)ptr, (uintptr_t)ptr, (intptr_t)ptr)

IM3Memory m3_InitMemory(IM3Memory memory) {
    if (!memory) return NULL;
    
    // Inizializza strutture base
    memory->segments = m3_Def_AllocArray(MemorySegment*, 1);
    if (!memory->segments) return NULL;
    
    memory->max_size = WASM_SEGMENT_SIZE * M3Memory_MaxPages;
    memory->num_segments = 0;
    memory->total_size = 0;
    memory->segment_size = WASM_SEGMENT_SIZE;
    memory->maxPages = M3Memory_MaxPages;
    
    // Inizializza cache dei chunk liberi
    memory->num_free_buckets = 32;
    memory->free_chunks = calloc(memory->num_free_buckets, sizeof(MemoryChunk*));
    if (!memory->free_chunks) {
        m3_Def_Free(memory->segments);
        return NULL;
    }
    
    // Alloca primo segmento
    M3Result result = AddSegment(memory, WASM_INIT_SEGMENTS);
    if (result != m3Err_none) {
        ESP_LOGE("WASM3", "m3_InitMemory: Failed to add segment: %s", result);
        m3_Def_Free(memory->segments);
        free(memory->free_chunks);
        return NULL;
    }
    
    // Inizializza il primo chunk nel primo segmento
    MemorySegment* first_seg = memory->segments[0];
    if (!first_seg || !first_seg->data) {
        m3_Def_Free(memory->segments);
        free(memory->free_chunks);
        return NULL;
    }
    
    MemoryChunk* first_chunk = (MemoryChunk*)first_seg->data;
    first_chunk->size = first_seg->size;
    first_chunk->is_free = true;
    first_chunk->next = NULL;
    first_chunk->prev = NULL;
    first_seg->first_chunk = first_chunk;
    
    // Aggiungi il primo chunk alla cache
    size_t bucket = log2(first_chunk->size);
    if (bucket < memory->num_free_buckets) {
        first_chunk->next = NULL;
        memory->free_chunks[bucket] = first_chunk;
    }

    /// Test init
    u32 ptr1 = m3_malloc(memory, 1);
    u32 ptr2 = m3_malloc(memory, 1);

    PRINT_PTR(ptr1);
    PRINT_PTR(ptr2);
    
    return memory;
}


void* m3_malloc(IM3Memory memory, size_t size) {
    if (!memory || size == 0) return NULL;
    
    // Include header size nella dimensione richiesta
    size_t total_size = size + sizeof(MemoryChunk);
    size_t aligned_size = align_size(total_size);
    size_t bucket = log2(aligned_size);
    
    // Debug log
    if (WASM_DEBUG_SEGMENTED_MEMORY) {
        ESP_LOGI("WASM3", "m3_malloc: requesting size=%zu, aligned_size=%zu, bucket=%zu", 
                 size, aligned_size, bucket);
    }
    
    // Cerca chunk libero nella cache
    MemoryChunk* found_chunk = NULL;
    if (bucket < memory->num_free_buckets) {
        MemoryChunk** curr = &memory->free_chunks[bucket];
        while (*curr) {
            if ((*curr)->size >= aligned_size) {
                found_chunk = *curr;
                *curr = found_chunk->next;  // Rimuovi dalla lista dei liberi
                break;
            }
            curr = &(*curr)->next;
        }
    }
    
    // Se non trovato in cache, cerca nei segmenti
    if (!found_chunk) {
        for (size_t i = 0; i < memory->num_segments && !found_chunk; i++) {
            MemorySegment* seg = memory->segments[i];
            if (!seg || !seg->data) continue;
            
            MemoryChunk* chunk = seg->first_chunk;
            while (chunk) {
                if (chunk->is_free && chunk->size >= aligned_size) {
                    found_chunk = chunk;
                    // Rimuovi dalla cache se presente
                    size_t chunk_bucket = log2(chunk->size);
                    if (chunk_bucket < memory->num_free_buckets) {
                        MemoryChunk** curr = &memory->free_chunks[chunk_bucket];
                        while (*curr && *curr != chunk) curr = &(*curr)->next;
                        if (*curr) *curr = chunk->next;
                    }
                    break;
                }
                chunk = chunk->next;
            }
        }
    }
    
    // Se ancora non trovato, alloca nuovo segmento
    if (!found_chunk) {
        size_t needed_segments = (aligned_size + memory->segment_size - 1) / memory->segment_size;
        if(needed_segments > memory->segment_size){
            if(WASM_DEBUG_SEGMENTED_MEM_MAN)
                ESP_LOGI("WASM3", "m3_malloc: requested segment size %d", needed_segments);
            
            M3Result result = AddSegment(memory, needed_segments);
            if (result != m3Err_none) return NULL;
        }
        
        MemorySegment* new_seg = memory->segments[memory->num_segments - 1];
        if (!new_seg || !new_seg->data) return NULL;
        
        found_chunk = (MemoryChunk*)new_seg->data;
        found_chunk->size = new_seg->size;
        found_chunk->is_free = true;
        found_chunk->next = NULL;
        found_chunk->prev = NULL;
        new_seg->first_chunk = found_chunk;
    }
    
    // Split chunk se necessario
    if (found_chunk->size > aligned_size + sizeof(MemoryChunk) + WASM_CHUNK_SIZE) {
        size_t remaining_size = found_chunk->size - aligned_size;
        MemoryChunk* new_chunk = (MemoryChunk*)((char*)found_chunk + aligned_size);
        
        new_chunk->size = remaining_size;
        new_chunk->is_free = true;
        new_chunk->next = found_chunk->next;
        new_chunk->prev = found_chunk;
        
        found_chunk->size = aligned_size;
        found_chunk->next = new_chunk;
        
        // Aggiungi nuovo chunk alla cache
        size_t new_bucket = log2(new_chunk->size);
        if (new_bucket < memory->num_free_buckets) {
            new_chunk->next = memory->free_chunks[new_bucket];
            memory->free_chunks[new_bucket] = new_chunk;
        }
    }
    
    // Marca come allocato e restituisci puntatore ai dati
    found_chunk->is_free = false;
    
    if (WASM_DEBUG_SEGMENTED_MEMORY) {
        ESP_LOGI("WASM3", "m3_malloc: returning ptr=%p, chunk=%p", 
                 get_chunk_data(found_chunk), found_chunk);
    }
    
    return get_chunk_data(found_chunk);
}

// Realloc ottimizzata
void* m3_realloc(M3Memory* memory, void* ptr, size_t new_size) {
    if (!memory) return NULL;
    if (!ptr) return m3_malloc(memory, new_size);
    if (new_size == 0) {
        m3_free(memory, ptr);
        return NULL;
    }
    
    MemoryChunk* chunk = get_chunk_header(ptr);
    size_t aligned_new_size = align_size(new_size);
    
    // Se la nuova size è minore, split il chunk
    if (aligned_new_size <= chunk->size) {
        if (chunk->size >= aligned_new_size + WASM_CHUNK_SIZE) {
            MemoryChunk* new_chunk = (MemoryChunk*)((char*)chunk + aligned_new_size);
            new_chunk->size = chunk->size - aligned_new_size;
            new_chunk->is_free = true;
            new_chunk->next = chunk->next;
            new_chunk->prev = chunk;
            
            chunk->size = aligned_new_size;
            chunk->next = new_chunk;
            
            // Aggiungi alla cache
            size_t bucket = log2(new_chunk->size);
            if (bucket < memory->num_free_buckets) {
                new_chunk->next = memory->free_chunks[bucket];
                memory->free_chunks[bucket] = new_chunk;
            }
        }
        return ptr;
    }
    
    // Prova a unire con chunk successivo se libero
    if (chunk->next && chunk->next->is_free && 
        chunk->size + chunk->next->size >= aligned_new_size) {
        // Rimuovi chunk successivo dalla cache
        size_t bucket = log2(chunk->next->size);
        if (bucket < memory->num_free_buckets) {
            MemoryChunk** curr = &memory->free_chunks[bucket];
            while (*curr && *curr != chunk->next) curr = &(*curr)->next;
            if (*curr) *curr = chunk->next->next;
        }
        
        chunk->size += chunk->next->size;
        chunk->next = chunk->next->next;
        if (chunk->next) chunk->next->prev = chunk;
        
        // Split se necessario
        if (chunk->size >= aligned_new_size + WASM_CHUNK_SIZE) {
            MemoryChunk* new_chunk = (MemoryChunk*)((char*)chunk + aligned_new_size);
            new_chunk->size = chunk->size - aligned_new_size;
            new_chunk->is_free = true;
            new_chunk->next = chunk->next;
            new_chunk->prev = chunk;
            
            chunk->size = aligned_new_size;
            chunk->next = new_chunk;
            
            // Aggiungi alla cache
            bucket = log2(new_chunk->size);
            if (bucket < memory->num_free_buckets) {
                new_chunk->next = memory->free_chunks[bucket];
                memory->free_chunks[bucket] = new_chunk;
            }
        }
        return ptr;
    }
    
    // Alloca nuovo chunk e copia dati
    void* new_ptr = m3_malloc(memory, new_size);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, ptr, chunk->size - sizeof(MemoryChunk));
    m3_free(memory, ptr);
    return new_ptr;
}

// Free con supporto alla coalescenza e cache
void m3_free(M3Memory* memory, void* ptr) {
    if (!memory || !ptr) return;
    
    MemoryChunk* chunk = get_chunk_header(ptr);
    chunk->is_free = true;
    
    // Unisci con chunk adiacenti se liberi
    if (chunk->next && chunk->next->is_free) {
        // Rimuovi dalla cache
        size_t bucket = log2(chunk->next->size);
        if (bucket < memory->num_free_buckets) {
            MemoryChunk** curr = &memory->free_chunks[bucket];
            while (*curr && *curr != chunk->next) curr = &(*curr)->next;
            if (*curr) *curr = chunk->next->next;
        }
        
        chunk->size += chunk->next->size;
        chunk->next = chunk->next->next;
        if (chunk->next) chunk->next->prev = chunk;
    }
    
    if (chunk->prev && chunk->prev->is_free) {
        // Rimuovi dalla cache
        size_t bucket = log2(chunk->prev->size);
        if (bucket < memory->num_free_buckets) {
            MemoryChunk** curr = &memory->free_chunks[bucket];
            while (*curr && *curr != chunk->prev) curr = &(*curr)->next;
            if (*curr) *curr = chunk->prev->next;
        }
        
        chunk->prev->size += chunk->size;
        chunk->prev->next = chunk->next;
        if (chunk->next) chunk->next->prev = chunk->prev;
        chunk = chunk->prev;
    }
    
    // Aggiungi alla cache
    size_t bucket = log2(chunk->size);
    if (bucket < memory->num_free_buckets) {
        chunk->next = memory->free_chunks[bucket];
        memory->free_chunks[bucket] = chunk;
    }
}