#include "esp_log.h"

#include "m3_exception.h"


char* error_details(const char* base_error, const char* format, ...) {
    static char buffer[512];  // Buffer statico per il risultato
    char temp_buffer[256];    // Buffer temporaneo per la parte formattata
    va_list args;
    
    // Formatta la seconda parte con i parametri variabili
    va_start(args, format);
    vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);
    va_end(args);
    
    // Combina l'errore base con i dettagli formattati
    snprintf(buffer, sizeof(buffer), "%s: %s", base_error, temp_buffer);
    
    return buffer;
}

void custom_panic_handler(void* frame, panic_info_t* info) {
    ESP_LOGE("WASM3", "Custom panic handler");
    esp_backtrace_print(100);
}

///
///
///

void print_last_two_callers() {
    #define MAX_BACKTRACE_SIZE 3
    const char* TAG = "WASM3";

    uint32_t pc, sp, next_pc;
    esp_backtrace_frame_t frame;
    esp_backtrace_frame_t frames[3];
    int frame_count = 0;

    // Ottiene il primo frame
    esp_backtrace_get_start(&pc, &sp, &next_pc);
    frame.pc = pc;
    frame.sp = sp;
    frame.next_pc = next_pc;
    frame.exc_frame = NULL;

    // Memorizza i primi 3 frame (incluso quello corrente)
    while (frame_count < 3 && frame.next_pc != 0) {
        frames[frame_count++] = frame;
        if (!esp_backtrace_get_next_frame(&frame)) {
            ESP_LOGW(TAG, "Errore nell'ottenere il frame successivo");
            break;
        }
    }

    // Se abbiamo almeno 3 frame, stampiamo il secondo e il terzo
    // (escludendo il frame corrente)
    if (frame_count >= 2) {
        ESP_LOGI(TAG, "Ultime due funzioni chiamanti:");
        printf("\nBacktrace:");
        for (int i = 1; i < frame_count && i <= 2; i++) {
            printf(" 0x%08x:0x%08x", frames[i].pc, frames[i].sp);
        }
        printf("\n");
    } else {
        ESP_LOGW(TAG, "Non ci sono abbastanza frame per mostrare le ultime due funzioni chiamanti");
    }
}

void nothing_todo(){}