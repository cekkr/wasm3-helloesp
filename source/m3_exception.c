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
