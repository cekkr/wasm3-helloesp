#ifndef m3_esp_try_h
#define m3_esp_try_h

#include <setjmp.h>
#include "esp_err.h"
#include "esp_log.h"

#include "wasm3.h"

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

void backtrace();

m3ret_t backtrace_err(m3ret_t err);

// Esempio di funzione che restituisce un errore
esp_err_t faulty_function();

#endif