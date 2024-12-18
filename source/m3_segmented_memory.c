#include "m3_segmented_memory.h"
#include "esp_log.h"
#include "m3_pointers.h"

#define WASM_DEBUG_SEGMENTED_MEM_MAN 1


#define PRINT_PTR(ptr) ESP_LOGI("WASM3", "Pointer value: (unsigned: %u, signed: %d)", (uintptr_t)ptr, (intptr_t)ptr)

const bool DEBUG_WASM_INIT_MEMORY = true;
IM3Memory m3_InitMemory(IM3Memory memory) {
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory called");

    if (!memory) {
        if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: memory pointer is NULL");
        return NULL;
    }

    memory->firm = INIT_FIRM;
    
    // Inizializza strutture base
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: allocating first segment");
    memory->segments = NULL;

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
    // Inizializza i campi base
    first_chunk->size = first_seg->size;
    first_chunk->is_free = true;
    first_chunk->next = NULL;
    first_chunk->prev = NULL;
    
    // Inizializza i nuovi campi per multi-segment
    first_chunk->num_segments = 1;  // Il primo chunk occupa un solo segmento
    first_chunk->start_segment = 0; // Inizia dal segmento 0
    first_chunk->segment_sizes = m3_Def_Malloc(sizeof(size_t)); // Alloca array per un segmento
    if (!first_chunk->segment_sizes) {
        ESP_LOGE("WASM3", "m3_InitMemory: Failed to allocate segment_sizes");
        m3_Def_Free(memory->segments);
        free(memory->free_chunks);
        return NULL;
    }
    first_chunk->segment_sizes[0] = first_seg->size; // Dimensione del primo segmento
    
    first_seg->first_chunk = first_chunk;
    
    // Aggiungi il primo chunk alla cache
    size_t bucket = log2(first_chunk->size);
    if (bucket < memory->num_free_buckets) {
        first_chunk->next = NULL;
        memory->free_chunks[bucket] = first_chunk;
    }    

    /// Test init
    if(DEBUG_WASM_INIT_MEMORY) ESP_LOGI("WASM3", "m3_InitMemory: test m3_malloc");
    u32 ptr1 = m3_malloc(memory, 1);
    u32 ptr2 = m3_malloc(memory, 1);

    if(DEBUG_WASM_INIT_MEMORY) {
        ESP_LOGI("WASM3", "m3_InitMemory: First 2 pointers init result:");
        PRINT_PTR(ptr1);
        PRINT_PTR(ptr2);
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
    if (!memory) return m3Err_nullMemory;

    bool updateMemory = false;

    if(seg == NULL){
        seg = m3_Def_Malloc(sizeof(MemorySegment));
    }

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
        seg->size = memory->segment_size;
        memory->total_allocated_size += memory->segment_size;      
    }        

    return NULL;
}

M3Result AddSegments(M3Memory* memory, size_t set_num_segments) {
    if(memory->firm != INIT_FIRM){
        ESP_LOGE("WASM3", "AddSegments: memory's firm is wrong");
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
    else {
        return NULL;
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

    memory->total_size = memory->segment_size * new_segments;
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
const bool WASM_DEBUG_GET_SEGMENT_POINTER = true;
const bool WASM_DEBUG_GET_SEGMENT_POINTER_NULLMEMORY_BACKTRACE = false;

////////////////////////////////////////////////////////////////
///////////////////////// Unused functions /////////////////////
////////////////////////////////////////////////////////////////

// Modifica della funzione get_chunk_header per supportare multi-segment
static MemoryChunk* get_chunk_header(void* ptr) { //todo: unused
    if (!ptr) return NULL;
    
    // Se il puntatore è all'interno di un chunk multi-segmento,
    // dobbiamo trovare l'header nel primo segmento
    IM3Memory memory = NULL; // Questo dovrebbe essere passato come parametro o accessibile globalmente
    
    for (size_t i = 0; i < memory->num_segments; i++) {
        MemorySegment* seg = memory->segments[i];
        if (!seg || !seg->data) continue;
        
        MemoryChunk* chunk = seg->first_chunk;
        while (chunk) {
            // Calcola l'area dati del chunk corrente
            void* chunk_data = (void*)((char*)chunk + sizeof(MemoryChunk));
            size_t total_size = 0;
            
            // Per chunk multi-segmento, calcola la dimensione totale
            for (size_t j = 0; j < chunk->num_segments; j++) {
                total_size += chunk->segment_sizes[j];
            }
            
            // Verifica se il puntatore è all'interno di questo chunk
            if (ptr >= chunk_data && ptr < (void*)((char*)chunk_data + total_size)) {
                return chunk;
            }
            
            chunk = chunk->next;
        }
    }
    
    // Se non troviamo il chunk, assumiamo che sia un puntatore normale
    return (MemoryChunk*)((char*)ptr - sizeof(MemoryChunk));
}

// Funzione aggiornata per gestire i chunk multi-segmento
static void* get_chunk_data(MemoryChunk* chunk) { //todo: unused
    if (!chunk) return NULL;
    return (void*)((char*)chunk + sizeof(MemoryChunk));
}

// Nuova funzione per calcolare l'offset totale in un chunk multi-segmento
static size_t get_chunk_offset(MemoryChunk* chunk, void* ptr) { //todo: unused
    if (!chunk || !ptr) return 0;
    
    size_t offset = 0;
    void* chunk_start = get_chunk_data(chunk);
    
    for (size_t i = 0; i < chunk->num_segments; i++) {
        void* segment_start = (void*)((char*)chunk_start + offset);
        void* segment_end = (void*)((char*)segment_start + chunk->segment_sizes[i]);
        
        if (ptr >= segment_start && ptr < segment_end) {
            return offset + ((char*)ptr - (char*)segment_start);
        }
        
        offset += chunk->segment_sizes[i];
    }
    
    return (size_t)-1; // Error case
}

// Funzione aggiornata per il calcolo della dimensione allineata
static size_t align_size(size_t size) { //todo: unused
    // Include spazio per l'header e l'array delle dimensioni dei segmenti
    size_t base_size = size + sizeof(MemoryChunk);
    size_t num_segments = (base_size + WASM_SEGMENT_SIZE - 1) / WASM_SEGMENT_SIZE;
    base_size += num_segments * sizeof(size_t); // Spazio per segment_sizes array
    
    return ((base_size + (WASM_CHUNK_SIZE-1)) & ~(WASM_CHUNK_SIZE-1));
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

// Funzione aggiornata per validare l'accesso alla memoria
bool IsValidMemoryAccess(IM3Memory memory, mos offset, size_t size) {
    if (!memory) return false;
    
    // Verifica se l'accesso è all'interno della memoria totale
    if (offset + size > memory->total_size) {
        return false;
    }
    
    // Per accessi che attraversano più segmenti
    size_t start_segment = offset / memory->segment_size;
    size_t end_segment = (offset + size - 1) / memory->segment_size;
    
    // Verifica che tutti i segmenti necessari esistano
    for (size_t i = start_segment; i <= end_segment; i++) {
        if (i >= memory->num_segments || !memory->segments[i]) {
            return false;
        }
        
        // Verifica che il segmento sia stato inizializzato
        if (!memory->segments[i]->data) {
            return false;
        }
    }
    
    return true;
}

// Funzione aggiornata per ottenere il puntatore a un segmento
void* get_segment_pointer(IM3Memory memory, u32 offset) {
    if (!memory || memory->firm != INIT_FIRM) {
        return offset;
    }
    
    // Calcola indici di segmento e offset
    size_t segment_index = offset / memory->segment_size;
    size_t segment_offset = offset % memory->segment_size;
    
    // Verifica validità del segmento
    if (segment_index >= memory->num_segments) {
        if (segment_index - memory->num_segments <= 2) {
            if (AddSegments(memory, segment_index) != NULL) {
                return ERROR_POINTER;
            }
        } else {
            return ERROR_POINTER;
        }
    }
    
    MemorySegment* seg = memory->segments[segment_index];
    if (!seg || seg->firm != INIT_FIRM) {
        return ERROR_POINTER;
    }
    
    // Assicurati che il segmento sia inizializzato
    if (!seg->data) {
        if (InitSegment(memory, seg, true)) {
            return ERROR_POINTER;
        }
    }
    
    // Gestione speciale per chunk multi-segmento
    MemoryChunk* chunk = seg->first_chunk;
    while (chunk) {
        if (chunk->num_segments > 1) {
            // Verifica se l'offset cade all'interno di questo chunk
            size_t chunk_offset = 0;
            for (size_t i = 0; i < chunk->num_segments; i++) {
                if (segment_index == chunk->start_segment + i) {
                    return (void*)((char*)seg->data + segment_offset);
                }
                chunk_offset += chunk->segment_sizes[i];
            }
        }
        chunk = chunk->next;
    }
    
    return (void*)((char*)seg->data + segment_offset);
}

const bool WASM_DEBUG_RESOLVE_POINTER_MEMORY_BACKTRACE = false;
static u32 resolve_pointer_cycle = 0;
void* resolve_pointer(IM3Memory memory, void* ptr) {
    if(is_ptr_valid(ptr)){
        if(WASM_DEBUG_MEM_ACCESS) ESP_LOG("WASM3", "resolve_pointer: ptr %p is valid as is", ptr);
        return ptr;
    }

    if(resolve_pointer_cycle++ % 3 == 0) { CALL_WATCHDOG }

    if(memory == NULL || memory->firm != INIT_FIRM) return ptr;

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

    if(IsValidMemoryAccess(memory, ptr, 0)){
        if(WASM_DEBUG_MEM_ACCESS) ESP_LOGW("WASM3", "resolve_pointer: ptr IsValidMemoryAccess");
        res = get_segment_pointer(memory, ptr);
    }
    else {
        if(WASM_DEBUG_MEM_ACCESS) ESP_LOGW("WASM3", "resolve_pointer: ptr NOT IsValidMemoryAccess");
        if(!is_ptr_valid(res)){
            ESP_LOGW("WASM3", "resolve_pointer: ptr is neither segment pointer neither valid ptr (ptr: %p, memory.total_size+req: %d)", 
                res, (memory->total_size + memory->total_requested_size));
        }
    }

    if(res == ERROR_POINTER || res == NULL){        
        res = ptr;
        if(WASM_DEBUG_MEM_ACCESS) ESP_LOGW("WASM3", "resolve_pointer: pointer not in segment (ptr: %p, res: %p)", ptr, res);
    }

    if(!is_ptr_valid(res)){
        ESP_LOGE("WASM3", "resolve_pointer: invalid pointer %p (from %p)", res, ptr);
        //LOG_FLUSH; backtrace(); // break the comment in case of necessity
        return res;
    }

    return res;
}

// Prendi l'arte e tienila da parte
void* resolve_pointer_impl2(IM3Memory memory, void* ptr) {
    // Se il puntatore è già valido, ritornalo così com'è
    if(is_ptr_valid(ptr)) {
        if(WASM_DEBUG_MEM_ACCESS) ESP_LOGI("WASM3", "resolve_pointer: ptr %p is valid as is", ptr);
        return ptr;
    }

    // Gestisci watchdog
    if(resolve_pointer_cycle++ % 3 == 0) { CALL_WATCHDOG }

    // Se memory non è valido, ritorna il puntatore originale
    if(memory == NULL || memory->firm != INIT_FIRM) {
        return ptr;
    }

    // Verifica validità della struttura memory
    CHECK_MEMORY_PTR(memory, "resolve_pointer");

    if(!is_ptr_valid(memory)) {
        ESP_LOGW("WASM3", "resolve_pointer: invalid memory pointer %p", memory);
        LOG_FLUSH;
        if(WASM_DEBUG_RESOLVE_POINTER_MEMORY_BACKTRACE) backtrace(); 
        return ptr;
    }

    if(WASM_DEBUG_MEM_ACCESS) {
        ESP_LOGI("WASM3", "resolve_pointer: memory ptr: %p, ptr: %p", memory, ptr);
        LOG_FLUSH;
    }

    // Considera il puntatore come un offset nella memoria WASM
    u32 offset = (u32)ptr;
    
    // Se l'offset è valido nella memoria WASM
    if(offset < memory->total_size) {
        void* resolved = get_segment_pointer(memory, offset);
        if(resolved != ERROR_POINTER && is_ptr_valid(resolved)) {
            return resolved;
        }
    }

    // Se arriviamo qui, il puntatore non è né un offset valido né un puntatore valido
    ESP_LOGW("WASM3", "resolve_pointer: ptr is neither segment pointer neither valid ptr (ptr: %p, memory.total_size+req: %d)", 
        ptr, (memory->total_size + memory->total_requested_size));
    
    // In caso di fallimento, ritorna il puntatore originale
    return ptr;
}

const bool WASM_DEBUG_MEM_ACCESS_BACKTRACE = false;

void* m3SegmentedMemAccess(IM3Memory memory, void* offset, size_t size) {
    return resolve_pointer(memory, (void*)offset);
}

void* m3SegmentedMemAccess__old(IM3Memory mem, void* ptr, size_t size) 
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

#define MIN(x, y) ((x) < (y) ? (x) : (y)) // is it used?

// Debug flag for m3_memcpy
const bool WASM_DEBUG_MEMCPY = false;

// Helper function to find chunk from pointer
static MemoryChunk* find_chunk_from_ptr(M3Memory* mem, void* ptr) {
    if (!mem || !ptr) return NULL;
    
    for (size_t i = 0; i < mem->num_segments; i++) {
        MemorySegment* seg = mem->segments[i];
        if (!seg || !seg->data) continue;
        
        // Check if pointer is within this segment
        if (ptr >= seg->data && ptr < (void*)((char*)seg->data + seg->size)) {
            MemoryChunk* chunk = seg->first_chunk;
            while (chunk) {
                void* chunk_data = (void*)((char*)chunk + sizeof(MemoryChunk));
                if (ptr >= chunk_data && ptr < (void*)((char*)chunk_data + chunk->size)) {
                    return chunk;
                }
                chunk = chunk->next;
            }
        }
    }
    return NULL;
}

void m3_memcpy(M3Memory* memory, void* dest, const void* src, size_t n) {
    if (!memory || !dest || !src || !n) return;
    
    // Check if memory is valid
    if (memory->firm != INIT_FIRM) {
        if (WASM_DEBUG_MEMCPY) ESP_LOGI("WASM3", "m3_memcpy: using standard memcpy (invalid memory)");
        memcpy(dest, src, n);
        return;
    }

    // Convert pointers to their actual memory locations if needed
    void* real_src = (void*)src;
    void* real_dest = dest;
    
    // Flags to track if source/dest are in segmented memory
    bool src_in_memory = false;
    bool dest_in_memory = false;
    
    // Check if source is a memory offset
    if (IsValidMemoryAccess(memory, (u64)src, n)) {
        src_in_memory = true;
        real_src = get_segment_pointer(memory, (u32)src);
        if (real_src == ERROR_POINTER) {
            if (WASM_DEBUG_MEMCPY) ESP_LOGE("WASM3", "m3_memcpy: invalid source offset");
            return;
        }
    } else {
        // Try to resolve as a pointer if not a valid offset
        real_src = resolve_pointer(memory, (void*)src);
        if (real_src == ERROR_POINTER) {
            // If not in memory segments, use original pointer
            real_src = (void*)src;
        } else {
            src_in_memory = true;
        }
    }
    
    // Check if destination is a memory offset
    if (IsValidMemoryAccess(memory, (u64)dest, n)) {
        dest_in_memory = true;
        real_dest = get_segment_pointer(memory, (u32)dest);
        if (real_dest == ERROR_POINTER) {
            if (WASM_DEBUG_MEMCPY) ESP_LOGE("WASM3", "m3_memcpy: invalid destination offset");
            return;
        }
    } else {
        // Try to resolve as a pointer if not a valid offset
        real_dest = resolve_pointer(memory, dest);
        if (real_dest == ERROR_POINTER) {
            // If not in memory segments, use original pointer
            real_dest = dest;
        } else {
            dest_in_memory = true;
        }
    }

    if (WASM_DEBUG_MEMCPY) {
        ESP_LOGI("WASM3", "m3_memcpy: src=%p->%p (in_mem=%d), dest=%p->%p (in_mem=%d), size=%zu",
                 src, real_src, src_in_memory, dest, real_dest, dest_in_memory, n);
    }

    // Find chunks if pointers are in segmented memory
    MemoryChunk* src_chunk = src_in_memory ? find_chunk_from_ptr(memory, real_src) : NULL;
    MemoryChunk* dest_chunk = dest_in_memory ? find_chunk_from_ptr(memory, real_dest) : NULL;

    // Handle different copy scenarios
    if (!src_chunk && !dest_chunk) {
        // Neither source nor destination are in multi-segment chunks
        memcpy(real_dest, real_src, n);
    } else {
        // At least one is a multi-segment chunk
        size_t remaining = n;
        char* curr_src = (char*)real_src;
        char* curr_dest = (char*)real_dest;

        while (remaining > 0) {
            size_t copy_size = remaining;

            if (src_chunk && src_chunk->num_segments > 1) {
                // Calculate remaining size in current source segment
                size_t src_seg_idx = (curr_src - (char*)memory->segments[src_chunk->start_segment]->data) 
                                   / memory->segment_size;
                size_t src_seg_offset = (curr_src - (char*)memory->segments[src_chunk->start_segment]->data) 
                                      % memory->segment_size;
                copy_size = MIN(copy_size, src_chunk->segment_sizes[src_seg_idx] - src_seg_offset);
            }

            if (dest_chunk && dest_chunk->num_segments > 1) {
                // Calculate remaining size in current destination segment
                size_t dest_seg_idx = (curr_dest - (char*)memory->segments[dest_chunk->start_segment]->data) 
                                    / memory->segment_size;
                size_t dest_seg_offset = (curr_dest - (char*)memory->segments[dest_chunk->start_segment]->data) 
                                       % memory->segment_size;
                copy_size = MIN(copy_size, dest_chunk->segment_sizes[dest_seg_idx] - dest_seg_offset);
            }

            // Perform the copy for this segment
            memcpy(curr_dest, curr_src, copy_size);
            
            // Update pointers and remaining size
            curr_src += copy_size;
            curr_dest += copy_size;
            remaining -= copy_size;

            // If source is multi-segment, check if we need to move to next segment
            if (src_chunk && src_chunk->num_segments > 1 && remaining > 0) {
                size_t seg_idx = (curr_src - (char*)memory->segments[src_chunk->start_segment]->data) 
                                / memory->segment_size;
                if (seg_idx < src_chunk->num_segments - 1) {
                    curr_src = (char*)memory->segments[src_chunk->start_segment + seg_idx + 1]->data;
                }
            }

            // If destination is multi-segment, check if we need to move to next segment
            if (dest_chunk && dest_chunk->num_segments > 1 && remaining > 0) {
                size_t seg_idx = (curr_dest - (char*)memory->segments[dest_chunk->start_segment]->data) 
                                / memory->segment_size;
                if (seg_idx < dest_chunk->num_segments - 1) {
                    curr_dest = (char*)memory->segments[dest_chunk->start_segment + seg_idx + 1]->data;
                }
            }
        }
    }
}

///
///
///

const bool WASM_DEBUG_SEGMENTED_MEMORY_ALLOC = true;

// Helper functions for multi-segment chunks
static MemoryChunk* create_multi_segment_chunk(M3Memory* memory, size_t size, size_t start_segment) {
    if(start_segment >= memory->num_segments)
        AddSegments(memory, start_segment + size);

    if (!memory || !memory->segments || start_segment >= memory->num_segments) {
        return NULL;
    }

    // Assicuriamoci che il primo segmento sia inizializzato
    MemorySegment* first_seg = memory->segments[start_segment];
    if (!first_seg) {
        return NULL;
    }

    if (!first_seg->data) {        
        if (InitSegment(memory, first_seg, true) != NULL) {
            return NULL;
        }
    }

    // Il resto del codice rimane identico
    size_t remaining_size = size;
    size_t num_segments = 0;
    size_t current_segment = start_segment;
    
    while (remaining_size > 0 && current_segment < memory->num_segments) {
        size_t available = memory->segment_size - sizeof(MemoryChunk);
        if (num_segments == 0) {
            available = memory->segment_size - sizeof(MemoryChunk);
        }
        
        remaining_size -= MIN(remaining_size, available);
        num_segments++;
        current_segment++;
    }
    
    if (remaining_size > 0) {
        return NULL;
    }
    
    MemoryChunk* chunk = (MemoryChunk*)first_seg->data;
    if (!chunk) {
        return NULL;
    }

    // Inizializziamo prima i campi più semplici
    chunk->size = size;
    chunk->is_free = false;
    chunk->next = NULL;
    chunk->prev = NULL;
    
    // Allocate and fill segment sizes array prima di settare num_segments
    size_t* sizes = m3_Def_Malloc(sizeof(size_t) * num_segments);
    if (!sizes) {
        return NULL;
    }
    
    chunk->segment_sizes = sizes;  // Settiamo prima l'array
    chunk->start_segment = start_segment;
    chunk->num_segments = num_segments;  // Ora possiamo settare num_segments
    
    // Calculate sizes for each segment
    remaining_size = size;
    for (size_t i = 0; i < num_segments; i++) {
        size_t available = memory->segment_size;
        if (i == 0) {
            available -= sizeof(MemoryChunk);
        }
        
        chunk->segment_sizes[i] = MIN(remaining_size, available);
        remaining_size -= chunk->segment_sizes[i];
    }
    
    return chunk;
}

///
///
///

// Helper per convertire da puntatore a offset
static u32 ptr_to_offset(M3Memory* memory, void* ptr) {
    if (!memory || !ptr) return 0;
    
    for (size_t i = 0; i < memory->num_segments; i++) {
        MemorySegment* seg = memory->segments[i];
        if (!seg || !seg->data) continue;
        
        if (ptr >= seg->data && ptr < (void*)((char*)seg->data + seg->size)) {
            return (i * memory->segment_size) + ((char*)ptr - (char*)seg->data);
        }
    }
    
    return 0;
}

void* m3_malloc(M3Memory* memory, size_t size) {
    if (!memory || size == 0) return NULL;
    
    size_t total_size = size + sizeof(MemoryChunk);
    
    // Try to find existing free chunk first
    size_t bucket = log2(total_size);
    MemoryChunk* found_chunk = NULL;
    
    if (bucket < memory->num_free_buckets) {
        MemoryChunk** curr = &memory->free_chunks[bucket];
        while (*curr) {
            if ((*curr)->size >= total_size) {
                found_chunk = *curr;
                *curr = found_chunk->next;
                break;
            }
            curr = &(*curr)->next;
        }
    }
    
    if (!found_chunk) {
        // Need to create new chunk
        size_t start_segment = 0;
        bool found_start = false;
        
        // Find first segment with enough space
        for (size_t i = 0; i < memory->num_segments; i++) {
            if (!memory->segments[i]->first_chunk) {
                start_segment = i;
                found_start = true;
                break;
            }
        }
        
        if (!found_start) {
            // Need new segments
            size_t needed_segments = (total_size + memory->segment_size - 1) / memory->segment_size;
            M3Result result = AddSegments(memory, needed_segments);
            if (result != m3Err_none) {
                return NULL;
            }
            start_segment = memory->num_segments - needed_segments;
        }
        
        found_chunk = create_multi_segment_chunk(memory, total_size, start_segment);
        if (!found_chunk) {
            return NULL;
        }
    }
    
    // Calcola l'offset relativo invece del puntatore assoluto
    u32 offset = ptr_to_offset(memory, found_chunk);
    return (void*)(offset + sizeof(MemoryChunk));
}

void m3_free(M3Memory* memory, void* offset_ptr) {
    if (!memory || !offset_ptr) return;
    
    // Converti l'offset in puntatore assoluto per le operazioni interne
    u32 offset = (u32)offset_ptr;
    void* real_ptr = get_segment_pointer(memory, offset - sizeof(MemoryChunk));
    if (real_ptr == ERROR_POINTER) return;
    
    MemoryChunk* chunk = (MemoryChunk*)real_ptr;
    
    // Free segment sizes array for multi-segment chunks
    if (chunk->num_segments > 1) {
        m3_Def_Free(chunk->segment_sizes);
    }
    
    chunk->is_free = true;
    
    // Add to appropriate free bucket
    size_t bucket = log2(chunk->size);
    if (bucket < memory->num_free_buckets) {
        chunk->next = memory->free_chunks[bucket];
        memory->free_chunks[bucket] = chunk;
    }
}

static void copy_multi_segment_data(M3Memory* memory, MemoryChunk* dest_chunk, u32 src_offset, size_t size) {
    size_t copied = 0;
    size_t src_pos = 0;
    
    for (size_t i = 0; i < dest_chunk->num_segments && copied < size; i++) {
        size_t seg_idx = dest_chunk->start_segment + i;
        size_t copy_size = MIN(dest_chunk->segment_sizes[i], size - copied);
        
        void* dest_ptr = (char*)memory->segments[seg_idx]->data;
        if (i == 0) {
            dest_ptr = (char*)dest_ptr + sizeof(MemoryChunk);
        }
        
        void* src_ptr = get_segment_pointer(memory, src_offset + src_pos);
        if (src_ptr == ERROR_POINTER) break;
        
        memcpy(dest_ptr, src_ptr, copy_size);
        copied += copy_size;
        src_pos += copy_size;
    }
}

void* m3_realloc(M3Memory* memory, void* offset_ptr, size_t new_size) {
    if (!memory) return NULL;
    if (!offset_ptr) return m3_malloc(memory, new_size);
    if (new_size == 0) {
        m3_free(memory, offset_ptr);
        return NULL;
    }
    
    u32 offset = (u32)offset_ptr;
    void* real_ptr = get_segment_pointer(memory, offset - sizeof(MemoryChunk));
    if (real_ptr == ERROR_POINTER) return NULL;
    
    MemoryChunk* old_chunk = (MemoryChunk*)real_ptr;
    size_t total_new_size = new_size + sizeof(MemoryChunk);
    
    // If shrinking and still fits in current segments, just update size
    if (total_new_size <= old_chunk->size) {
        old_chunk->size = total_new_size;
        return offset_ptr;
    }
    
    // Allocate new multi-segment chunk
    void* new_offset = m3_malloc(memory, new_size);
    if (!new_offset) {
        return NULL;
    }
    
    // Copy data using offsets
    void* new_chunk_ptr = get_segment_pointer(memory, (u32)new_offset - sizeof(MemoryChunk));
    if (new_chunk_ptr == ERROR_POINTER) {
        m3_free(memory, new_offset);
        return NULL;
    }
    
    MemoryChunk* new_chunk = (MemoryChunk*)new_chunk_ptr;
    copy_multi_segment_data(memory, new_chunk, offset, MIN(old_chunk->size - sizeof(MemoryChunk), new_size));
    
    // Free old chunk
    m3_free(memory, offset_ptr);
    
    return new_offset;
}
