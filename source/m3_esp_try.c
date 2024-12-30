#include "m3_esp_try.h"

#include <setjmp.h>
#include "esp_err.h"
#include "esp_log.h"

#define TC_TAG "TC_WASM3"

// Variabile globale per salvare l'ultimo errore
static esp_err_t last_error;

void backtrace(){
    LOG_FLUSH; LOG_FLUSH;
    esp_backtrace_print(20);
    LOG_FLUSH; LOG_FLUSH;
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void waitForIt(){
    vTaskDelay(pdMS_TO_TICKS(100));
}


m3ret_t backtrace_err(m3ret_t err){
    //ESP_LOGE("WASM3", "Error: %s", err);
    if(err != NULL){
        //esp_backtrace_print(100); // enable when you need it
        ESP_LOGD("WASM3", "Result to check in backtrace_err");
    }

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
