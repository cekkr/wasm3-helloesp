#ifndef m3_esp_try_h
#define m3_esp_try_h

#include <setjmp.h>
#include "esp_err.h"
#include "esp_log.h"

#define TC_TAG "TC_WASM3"

// Variabile globale per salvare l'ultimo errore
static esp_err_t last_error;

// Definizione delle macro TRY-CATCH
#define TRY do { jmp_buf buf; if (!setjmp(buf)) {
#define CATCH } else {
#define END_TRY } } while (0)
#define THROW longjmp(buf, 1)

// Macro per controllare errori e lanciare un'eccezione
#define ESP_TRY_CHECK(x) do { \
    esp_err_t err_rc_ = (x); \
    if (err_rc_ != ESP_OK) { \
        last_error = err_rc_; /* Salva l'errore globale */ \
        THROW; \
    } \
} while (0)

void backtrace(){
    esp_backtrace_print(100);
}


M3Result backtrace_err(M3Result err){
    ESP_LOGE("WASM3", "Error: %s", err);
    esp_backtrace_print(100);
    return err;
}

// Esempio di funzione che restituisce un errore
esp_err_t faulty_function() {
    return ESP_FAIL; // Simula un errore
}

void example_function_trycatch() {
    TRY {
        ESP_LOGI(TC_TAG, "Inizio del blocco TRY");
        ESP_TRY_CHECK(faulty_function()); // Controllo e gestione errori
        ESP_LOGI(TC_TAG, "Questo messaggio non verrà mostrato se c'è un errore.");
    }
    CATCH {
        // Accesso all'ultimo errore e alla sua stringa
        ESP_LOGE(TC_TAG, "Eccezione catturata! Errore: %s (%d)", esp_err_to_name(last_error), last_error);
    }
    END_TRY;

    ESP_LOGI(TC_TAG, "Fine della funzione.");
}

#endif