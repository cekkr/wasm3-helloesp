#include "esp_debug_helpers.h"
#include "esp_heap_caps.h"

#ifndef m3_pointers_h
#define m3_pointers_h

///
/// Pointer validation
///

typedef enum {
    PTR_CHECK_OK = 0,
    PTR_CHECK_NULL,
    PTR_CHECK_UNALIGNED,
    PTR_CHECK_OUT_OF_BOUNDS
} ptr_check_result_t;

ptr_check_result_t validate_pointer(const void* ptr, size_t expected_size) {
    // 1. Controllo NULL pointer
    if (ptr == NULL) {
        ESP_LOGE("WASM3", "NULL pointer detected");
        return PTR_CHECK_NULL;
    }

    // 2. Verifica allineamento (assumendo che M3Memory richieda allineamento a 4 byte)
    if (((uintptr_t)ptr) % 4 != 0) {
        ESP_LOGE("WASM3", "Unaligned pointer: %p", ptr);
        return PTR_CHECK_UNALIGNED;
    }

    // 3. Verifica che il puntatore sia in un range di memoria valido
    // Questi valori vanno adattati in base alla configurazione della memoria dell'ESP32
    extern uint32_t _heap_start, _heap_end;
    if ((uintptr_t)ptr < (uintptr_t)&_heap_start || 
        (uintptr_t)ptr + expected_size > (uintptr_t)&_heap_end) {
        ESP_LOGE("WASM3", "Pointer out of valid memory range: %p", ptr);
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
        ESP_LOGW("WASM3", "Null pointer detected");
        return false;
    }

    // 2. Verifica allineamento (l'heap ESP32 richiede allineamento a 8 byte)
    if (((uintptr_t)ptr) % 8 != 0) {
        ESP_LOGE("WASM3", "Unaligned pointer: %p", ptr);
        return false;
    }

    // 3. Verifica che il puntatore sia nell'heap DRAM
    if (!heap_caps_check_integrity_addr((intptr_t)ptr, true)) {
        ESP_LOGE("WASM3", "Pointer not in valid heap region: %p", ptr);
        return false;
    }

    // 4. Verifica che il blocco di memoria abbia un header valido
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    if ((uintptr_t)ptr < 0 || (uintptr_t)ptr > (info.total_free_bytes + info.total_allocated_bytes)) {
        ESP_LOGE("WASM3", "Pointer outside heap bounds: %p", ptr);
        return false;
    }

    return true;
}

/**
 * @brief Libera la memoria in modo sicuro
 * @param ptr Puntatore da liberare
 * @return true se l'operazione è riuscita, false altrimenti
 */
bool safe_free(void** ptr) {
    if (ptr == NULL) {
        return false;
    }

    if (!is_ptr_freeable(*ptr)) {
        return false;
    }

    free(*ptr);
    *ptr = NULL;  // Previene use-after-free
    return true;
}

#endif  // m3_pointers_h