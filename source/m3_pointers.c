#include "esp_debug_helpers.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "m3_pointers.h"

static const bool WASM_DEBUG_POINTERS = false;
static const bool WASM_DEBUG_POINTERS_BACKTRACE = false;
static const bool WASM_DEBUG_POINTERS_IGNORE_OUTSIDE_HEAP = false; 
static const bool WASM_POINTERS_CHECK_BOUNDS = false;

ptr_check_result_t validate_pointer(const void* ptr, size_t expected_size) {
    // 1. Controllo NULL pointer
    if (ptr == NULL) {
        if(WASM_DEBUG_POINTERS) ESP_LOGE("WASM3", "NULL pointer detected");
        return PTR_CHECK_NULL;
    }

    // 2. Verifica allineamento (assumendo che M3Memory richieda allineamento a 4 byte)
    if (((uintptr_t)ptr) % 4 != 0) {
        if(WASM_DEBUG_POINTERS) ESP_LOGE("WASM3", "Unaligned pointer: %p", ptr);
        return PTR_CHECK_UNALIGNED;
    }

    // 3. Verifica che il puntatore sia in un range di memoria valido
    // Questi valori vanno adattati in base alla configurazione della memoria dell'ESP32
    extern uint32_t _heap_start, _heap_end;
    if ((uintptr_t)ptr < (uintptr_t)&_heap_start || 
        (uintptr_t)ptr + expected_size > (uintptr_t)&_heap_end) {
        if(WASM_DEBUG_POINTERS) ESP_LOGE("WASM3", "Pointer out of valid memory range: %p", ptr);
        return PTR_CHECK_OUT_OF_BOUNDS;
    }

    return PTR_CHECK_OK;
}

/**
 * @brief Verifica se un puntatore può essere liberato in sicurezza
 * @param ptr Puntatore da verificare
 * @return true se il puntatore è valido per free(), false altrimenti
 */
bool is_ptr_freeable(void* ptr) {
    // 1. Controllo puntatore NULL
    if (ptr == NULL) {
        if(WASM_DEBUG_POINTERS) ESP_LOGW("WASM3", "Null pointer detected");
        if(WASM_DEBUG_POINTERS_BACKTRACE) esp_backtrace_print(100);
        return false;
    }

    // 2. Verifica allineamento (l'heap ESP32 richiede allineamento a 8 byte)
    if (false && ((uintptr_t)ptr) % 8 != 0) { // disabled
        if(WASM_DEBUG_POINTERS){
             ESP_LOGE("WASM3", "Unaligned pointer: %p", ptr);
             if(WASM_DEBUG_POINTERS_BACKTRACE) esp_backtrace_print(100);
        }
        return false;
    }

    // 3. Verifica che il puntatore sia nell'heap DRAM
    if (!heap_caps_check_integrity_addr((intptr_t)ptr, true)) {
        if(WASM_DEBUG_POINTERS && WASM_DEBUG_POINTERS_IGNORE_OUTSIDE_HEAP) {
            ESP_LOGE("WASM3", "Pointer not in valid heap region: %p", ptr);
            if(WASM_DEBUG_POINTERS_BACKTRACE) esp_backtrace_print(100);
        }
        return false;
    }

    // 4. Verifica che il blocco di memoria abbia un header valido
    if(WASM_POINTERS_CHECK_BOUNDS){
        multi_heap_info_t info;
        heap_caps_get_info(&info, MALLOC_CAP_8BIT);
        if ((uintptr_t)ptr < 0 || (uintptr_t)ptr > (info.total_free_bytes + info.total_allocated_bytes)) {
            if(WASM_DEBUG_POINTERS) ESP_LOGE("WASM3", "Pointer outside heap bounds: %p", ptr);
            return false;
        }
    }

    return true;
}

/**
 * @brief Libera la memoria in modo sicuro
 * @param ptr Puntatore da liberare
 * @return true se l'operazione è riuscita, false altrimenti
 */
bool safe_free(void* ptr) {
    if (!(ptr)) {
        return false;
    }

    if (!is_ptr_freeable(ptr)) {
        return false;
    }

    //free(*ptr);
    //*ptr = NULL;  // Previene use-after-free
    return true;
}

///
///
///

typedef struct {
    void* ptr;
    size_t size;
    const char* allocation_point;
} safe_ptr_t;

// Verifica se un puntatore è valido per le operazioni di memoria
bool is_ptr_valid(const void* ptr) {
    if (!ptr) {
        ESP_LOGW(SAFE_MEM_TAG, "Null pointer detected");
        return false;
    }
    
    if (!esp_ptr_in_dram(ptr)) {
        ESP_LOGW(SAFE_MEM_TAG, "Pointer %p not in DRAM", ptr);
        return false;
    }
    
    if (!heap_caps_check_integrity_addr(ptr, true)) {
        ESP_LOGW(SAFE_MEM_TAG, "Heap corruption detected at %p", ptr);
        return false;
    }
    
    return true;
}

// Wrapper sicuro per free
bool safe_free_with_check(void** ptr) {
    if (!ptr || !(*ptr)) {
        ESP_LOGW(SAFE_MEM_TAG, "Attempting to free null pointer");
        return false;
    }
    
    if (!is_ptr_valid(*ptr)) {
        ESP_LOGW(SAFE_MEM_TAG, "Invalid pointer detected during free");
        *ptr = NULL;
        return false;
    }
    
    // Backup del puntatore per il logging
    void* original_ptr = *ptr;
    
    // Tentativo di free
    heap_caps_free(*ptr);
    
    // Clear del puntatore dopo la free
    *ptr = NULL;
    
    ESP_LOGD(SAFE_MEM_TAG, "Successfully freed memory at %p", original_ptr);
    return true;
}

// Wrapper sicuro per m3_Int_Free specifica per WASM3
bool safe_m3_int_free(void** ptr) {
    if (!ptr || !(*ptr)) {
        return false;
    }
    
    // Salva il valore originale per il logging
    void* original_ptr = *ptr;
    
    // Setup error handler
    if (setjmp(g_jmpBuf) == 0) {
        // Tenta l'operazione in un contesto protetto
        if (is_ptr_valid(*ptr)) {
            m3_Int_Free(*ptr);
            *ptr = NULL;
            ESP_LOGD(SAFE_MEM_TAG, "Successfully freed WASM3 memory at %p", original_ptr);
            return true;
        }
    } else {
        ESP_LOGE(SAFE_MEM_TAG, "Exception during WASM3 free operation at %p", original_ptr);
    }
    
    return false;
}

// Wrapper sicuro per Function_Release
bool safe_function_release(IM3Function* i_function) { // just for example purposes
    if (!i_function || !(*i_function)) {
        ESP_LOGW(SAFE_MEM_TAG, "Null function pointer");
        return false;
    }
    
    // Setup error handler
    if (setjmp(g_jmpBuf) == 0) {
        // Verifica validità della struttura funzione
        if (!is_ptr_valid(*i_function)) {
            ESP_LOGE(SAFE_MEM_TAG, "Invalid function pointer structure");
            return false;
        }
        
        // Gestione sicura dei constants
        if ((*i_function)->constants) {
            safe_m3_int_free((void**)&((*i_function)->constants));
        }
        
        // Gestione altri membri della struttura...
        
        // Free della struttura funzione
        safe_free_with_check((void**)i_function);
        return true;
    } else {
        ESP_LOGE(SAFE_MEM_TAG, "Exception during function release");
        return false;
    }
}