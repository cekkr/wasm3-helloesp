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


#define PRINT_PTR(ptr) ESP_LOGI("WASM3", "Pointer value: (unsigned: %u, signed: %d)", (uintptr_t)ptr, (intptr_t)ptr)

const bool DEBUG_WASM_INIT_MEMORY = true;
IM3Memory m3_InitMemory(IM3Memory memory) {
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory called");

    if (!memory) {
        if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: memory pointer is NULL");
        return NULL;
    }
    
    // Inizializza strutture base
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: allocating first segment");
    memory->segments = NULL; //m3_Def_AllocArray(MemorySegment*, 1);

    if (!memory->segments && false) {
        ESP_LOGE("WASM3", "m3_InitMemory: !memory->segment");
        return NULL;
    }
    
    memory->max_size = WASM_SEGMENT_SIZE * M3Memory_MaxPages;
    memory->num_segments = 0;
    memory->total_size = 0;
    memory->segment_size = WASM_SEGMENT_SIZE;
    memory->maxPages = M3Memory_MaxPages;
    
    // Inizializza cache dei chunk liberi
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: init free chunks");
    memory->num_free_buckets = 32;
    memory->free_chunks = calloc(memory->num_free_buckets, sizeof(MemoryChunk*));
    if (!memory->free_chunks) {
        m3_Def_Free(memory->segments);
        return NULL;
    }
    
    // Alloca primo segmento
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: AddSegments(WASM_INIT_SEGMENTS)");
    M3Result result = AddSegments(memory, WASM_INIT_SEGMENTS);
    if (result != m3Err_none) {
        ESP_LOGE("WASM3", "m3_InitMemory: Failed to add segment: %s", result);
        m3_Def_Free(memory->segments);
        free(memory->free_chunks);
        return NULL;
    }
    
    // Inizializza il primo chunk nel primo segmento
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: init first chunk's segment");
    MemorySegment* first_seg = memory->segments[0];
    if (first_seg) {
        if(!first_seg->data){
            // it's always null
            if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: first_seg->data is NULL");
            InitSegment(memory, first_seg, true);
        }
    }
    else {
        ESP_LOGE("WASM3", "m3_InitMemory: first_seg is NULL");
        m3_Def_Free(memory->segments);
        free(memory->free_chunks);
        return NULL;
    }
    
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: working on first_chunk");
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

    memory->firm = INIT_FIRM;

    /// Test init
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: test m3_malloc");
    u32 ptr1 = m3_malloc(memory, 1);
    u32 ptr2 = m3_malloc(memory, 1);

    PRINT_PTR(ptr1);
    PRINT_PTR(ptr2);
    
    return memory;
}


IM3Memory m3_NewMemory(){
    IM3Memory memory = m3_Def_AllocStruct (M3Memory);

    m3_InitMemory(memory);

    return memory;
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
    
    M3Result result = AddSegments(memory, additional_segments);
    if (result != NULL) {
        return result;
    }
    
    memory->total_size = new_total;

    m3_malloc(memory, 1); // Allocate memory
    
    return NULL;
}

// Funzione per aggiungere un nuovo segmento
const bool WASM_DEBUG_ADD_SEGMENT = false;

M3Result InitSegment(M3Memory* memory, MemorySegment* seg, bool initData){
    if (!memory ||!seg) return m3Err_nullMemory;

    bool updateMemory = false;
    if(seg->firm != INIT_FIRM){
        seg->firm = INIT_FIRM;  
        updateMemory = true;    
    }

    if(initData && seg->data == NULL){
        // Allocare i dati del segmento
        if(WASM_DEBUG_ADD_SEGMENT) ESP_LOGI("WASM3", "InitSegment: allocating segment data");
        seg->data = m3_Def_Malloc(memory->segment_size);
        if (!seg->data) {  
            ESP_LOGE("WASM3", "InitSegment: can't allocate segment data");          
            return m3Err_nullSegmentData;
        }

        seg->is_allocated = true;
    }        

    if(updateMemory){
        seg->size = memory->segment_size;
        memory->total_size += memory->segment_size;      
    }

    return NULL;
}

M3Result AddSegments(M3Memory* memory, size_t set_num_segments) {
    if(memory->segment_size == 0){
        ESP_LOGE("WASM3", "AddSegments: memory->segment_size is zero");
        backtrace();
        return m3Err_nullMemory;
    }

    size_t original_num_segments = memory->num_segments;

    size_t new_segments = set_num_segments;
    if(new_segments == 0){
        new_segments = memory->num_segments + 1;
    }
    else {
        new_segments++;
    }

    size_t new_size = (new_segments) * sizeof(MemorySegment*);
    
    if(new_segments > memory->num_segments){
        if(WASM_DEBUG_ADD_SEGMENT) ESP_LOGI("WASM3", "Adding segments (%d)", new_segments);
        MemorySegment** segments = m3_Def_Realloc(memory->segments, new_size);
        if (segments == NULL) {
            ESP_LOGE("WASM3", "AddSegments: failed to allocate new segments");
            memory->num_segments = original_num_segments;
            return m3Err_mallocFailed;
        }

        memory->segments = segments;
    }

    if(WASM_DEBUG_ADD_SEGMENT)
        ESP_LOGI("WASM3", "AddSegments: num_segments = %d, new_num_segments: %d", memory->num_segments, new_segments);

    size_t new_idx = memory->num_segments;
    for(; new_idx < new_segments; new_idx++){   
        if(WASM_DEBUG_ADD_SEGMENT){
            ESP_LOGI("WASM3", "AddSegments: memory->segments[%d]=%p", new_idx, memory->segments[new_idx]);
        }

        // Allocare la nuova struttura MemorySegment
        memory->segments[new_idx] = m3_Def_Malloc(sizeof(MemorySegment));
        if (!memory->segments[new_idx]) {
            ESP_LOGE("WASM3", "AddSegments: failed to allocate segment number %d", new_idx);
            goto backToOriginal;
        }                

        // Verifica che il puntatore sia valido prima di usarlo
        if (!ultra_safe_ptr_valid(memory->segments[new_idx])) {
            ESP_LOGE("WASM3", "AddSegments: Invalid pointer at memory->segments[%d]", new_idx);
            goto backToOriginal;
        }
        
        // Verifica che possiamo effettivamente accedere alla struttura
        MemorySegment* seg = memory->segments[new_idx];
        if(WASM_DEBUG_ADD_SEGMENT) ESP_LOGI("WASM3", "AddSegments: Can access segment structure");

        InitSegment(memory, seg, false);           
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

const bool WASM_DEBUG_SEGMENTED_MEM_ACCESS = false;
const bool WASM_DEBUG_MEM_ACCESS = false;
const bool WASM_DEBUG_GET_SEGMENT_POINTER = false;
const bool WASM_DEBUG_GET_SEGMENT_POINTER_NULLMEMORY_BACKTRACE = false;

static int get_segment_pointer_cycle = 0;
void* get_segment_pointer(IM3Memory memory, u32 offset) { 
    if(get_segment_pointer_cycle++ % 3 == 0) { CALL_WATCHDOG }

    if(memory == NULL){
        return offset;
    }

    CHECK_MEMORY_PTR(memory, "get_segment_pointer");

    if(memory->firm != INIT_FIRM){
        ESP_LOGW("WASM3", "get_segment_pointer: null memory (firm: %d)", memory->firm);
        if(WASM_DEBUG_GET_SEGMENT_POINTER_NULLMEMORY_BACKTRACE) backtrace();
        return offset;
    }

     if(memory->segments == NULL){
        ESP_LOGW("WASM3", "get_segment_pointer: memory not initialized");
        if(WASM_DEBUG_GET_SEGMENT_POINTER_NULLMEMORY_BACKTRACE) backtrace();
        return offset;
    }

    //CALL_WATCHDOG

    if(memory->firm != INIT_FIRM){
        ESP_LOGE("WASM3", "get_segment_pointer: memory firm not valid (%d)", memory->firm);
        backtrace(); LOG_FLUSH;
        goto failResult;
    }

    if(offset == (u32)ERROR_POINTER)
        return ERROR_POINTER;

    if (!memory || !memory->segments) {
        return ERROR_POINTER;
    }

    // Calculate segment index and offset
    size_t segment_index = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;

    // Check if segment exists and is valid
    if (segment_index >= memory->num_segments) {
        if(segment_index - memory->num_segments <= 2){
            if(AddSegments(memory, segment_index) != NULL){
                ESP_LOGW("WASM3", "get_segment_pointer: requested unallocated segment %d", segment_index);
                goto failResult;
            }
        }
        else {            
            goto failResult;
        }

        if(WASM_DEBUG_GET_SEGMENT_POINTER) ESP_LOGI("WASM3", "get_segment_pointer: get new segment index %d (num_segments: %d)", segment_index, memory->num_segments);
        MemorySegment* seg = memory->segments[segment_index];

        if(seg == NULL || seg->firm != INIT_FIRM){
            ESP_LOGE("WASM3", "get_segment_pointer: invalid segment %d", segment_index);
            goto failResult;
        }

        if(seg->data == NULL){
            if(InitSegment(memory, seg, true)){
                ESP_LOGE("WASM3", "get_segment_pointer: failed allocating segment %d", segment_index);
                goto failResult;
            }
        }        
    }

    MemorySegment* seg = memory->segments[segment_index];

    if(seg == NULL || seg->firm != INIT_FIRM){
        ESP_LOGE("WASM3", "get_segment_pointer: segment %d has invalid firm (%d), or is NULL", segment_index, seg->firm);
        backtrace(); LOG_FLUSH;
        goto failResult;
    }

    uint8_t* res = (uint8_t*)(seg->data + segment_offset);
    if(WASM_DEBUG_GET_SEGMENT_POINTER) ESP_LOGI("WASM3", "get_segment_pointer: M3Memory result: %p (from segment %d)", res, segment_index);
    return res;

    failResult:    
    return ERROR_POINTER;
}

const bool WASM_DEBUG_RESOLVE_POINTER_MEMORY_BACKTRACE = false;
void* resolve_pointer(IM3Memory memory, void* ptr) {
    if(memory == NULL) return ptr;

    CHECK_MEMORY_PTR(memory, "resolve_pointer");

    if(!is_ptr_valid(memory)){
        ESP_LOGW("WASM3", "resolve_pointer: invalid memory pointer %p", memory);
        LOG_FLUSH;
        if(WASM_DEBUG_RESOLVE_POINTER_MEMORY_BACKTRACE) backtrace(); 
        return ptr;
    }
    else {
        if(WASM_DEBUG_RESOLVE_POINTER_MEMORY_BACKTRACE){
            ESP_LOGW("WASM3", "resolve_pointer: seems valid memory pointer %p", memory);
            LOG_FLUSH;
            backtrace(); 
        }
    }

    if(WASM_DEBUG_MEM_ACCESS) {
        ESP_LOGI("WASM3", "resolve_pointer: memory ptr: %p, ptr: %p", memory, ptr);
        LOG_FLUSH;
    }

    void* res = ptr;

    if(IsValidMemoryAccess(memory, ptr, 1)){
        res = get_segment_pointer(memory, ptr);
    }

    if(res == ERROR_POINTER || res == NULL){        
        res = ptr;
        if(WASM_DEBUG_MEM_ACCESS) ESP_LOGW("WASM3", "resolve_pointer: pointer not in segment (ptr: %p, res: %p)", ptr, res);
    }

    if(!is_ptr_valid(res)){
        ESP_LOGE("WASM3", "resolve_pointer: invalid pointer %p (from %p)", res, ptr);
        //LOG_FLUSH;
        //backtrace();
        return ERROR_POINTER;
    }

    return res;
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
        if(WASM_DEBUG_MEM_ACCESS_BACKTRACE) backtrace();
        return ERROR_POINTER;
    }

    if(WASM_DEBUG_SEGMENTED_MEM_ACCESS){ 
        ESP_LOGI("WASM3", "m3SegmentedMemAccess call");         
        //ESP_LOGI("WASM3", "m3SegmentedMemAccess: mem = %p", (void*)mem);  
    }

    return resolve_pointer(mem, ptr);
}

void* m3SegmentedMemAccess_2(IM3Memory memory, u32 offset, size_t size) {
    return resolve_pointer(memory, (void*)offset);
}

bool IsValidMemoryAccess(IM3Memory memory, u64 offset, u32 size)
{
    if(memory == NULL) return false;

    //todo: improve precision by checking if it's inside allocated memory (?)
    return (offset + size) <= memory->total_size;
}

mos get_offset_pointer(IM3Memory memory, void* ptr) {
    if (!memory || !memory->segments || !ptr) {
        return (mos)ptr;
    }

    // If ptr is already an offset or ERROR_POINTER, return it as is
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

            if (WASM_DEBUG_GET_SEGMENT_POINTER) {
                ESP_LOGI("WASM3", "get_offset_pointer: converted %p to offset %u (segment %zu)", 
                         ptr, total_offset, i);
            }

            return total_offset;
        }
    }

    // If we didn't find the pointer in any segment, return the original pointer
    if (WASM_DEBUG_GET_SEGMENT_POINTER) {
        ESP_LOGI("WASM3", "get_offset_pointer: pointer %p not found in segmented memory", ptr);
    }
    
    return (mos)ptr;
}

////
////
////

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

const bool WASM_DEBUG_SEGMENTED_MEMORY = true;

void* m3_malloc(IM3Memory memory, size_t size) {    
    if (!memory || size == 0) return NULL;

    if(WASM_DEBUG_SEGMENTED_MEMORY) ESP_LOGI("WASM3", "m3_malloc: %d size", size);
    
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
            if (!seg) continue;

            if(!seg->data){
                if(InitSegment(memory, seg, true))
                    continue;
            }
            
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
            
            M3Result result = AddSegments(memory, needed_segments);
            if (result != m3Err_none) return NULL;
        }
        
        MemorySegment* new_seg = memory->segments[memory->num_segments - 1];
        if (!new_seg) return NULL;

        if(!new_seg->data){
            if(InitSegment(memory, new_seg, true)){
                return NULL;
            }
        }
        
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

    if(WASM_DEBUG_SEGMENTED_MEMORY) ESP_LOGI("WASM3", "m3_realloc: ptr: %p, new size: %d ",ptr, new_size);

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

    if(WASM_DEBUG_SEGMENTED_MEMORY) ESP_LOGI("WASM3", "m3_free: ptr: %p",ptr);
    
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