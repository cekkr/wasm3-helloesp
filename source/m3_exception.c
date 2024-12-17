#include "esp_log.h"
#include "esp_debug_helpers.h"

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

void print_last_two_callers() { //todo: deprecate it
    #define MAX_BACKTRACE_SIZE 3
    const char* TAG = "WASM3";

    esp_backtrace_frame_t frame;
    int frame_count = 0;

    // Ottieni il frame iniziale
    esp_backtrace_get_start(&frame.pc, &frame.sp, &frame.next_pc);

    esp_backtrace_frame_t frames[MAX_BACKTRACE_SIZE];

    // Itera e salva i frame
    while (frame_count < MAX_BACKTRACE_SIZE && frame.next_pc != 0) {
        frames[frame_count++] = frame;

        if (!esp_backtrace_get_next_frame(&frame)) {
            ESP_LOGW(TAG, "Errore nell'ottenere il frame successivo.");
            break;
        }
    }

    // Se ci sono almeno 2 frame successivi a quello corrente
    if (frame_count >= 3) {        
        ESP_LOGI(TAG, "Ultime due funzioni chiamanti:");

        int frame_up_to = 3;
        printf("Backtrace: ");
        // this frame's addresses extraction's wrong
        for (int i = 1; i <= frame_up_to && i < frame_count; i++) { // Inizia dal frame 1, saltando quello corrente
            printf("0x%08" PRIx32, frames[i].pc);
            printf(":");
            printf("0x%08" PRIx32, frames[i].sp);
        }
        printf("\n");
    } else {
        ESP_LOGW(TAG, "Non ci sono abbastanza frame per mostrare le ultime due funzioni chiamanti.");
    }
}

void nothing_todo(){}