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