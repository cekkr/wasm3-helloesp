//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#ifndef m3_exception_h
#define m3_exception_h

#include "m3_config.h"
#include "m3_segmented_memory.h"
#include "wasm3_defs.h"

# if d_m3EnableExceptionBreakpoint

// declared in m3_info.c
void ExceptionBreakpoint (cstr_t i_exception, cstr_t i_message);

#   define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

# else
#   define EXCEPTION_PRINT(...)
# endif


#define _try                                M3Result result = m3Err_none;
#define _(TRY)                              { result = TRY; if (M3_UNLIKELY(result)) { EXCEPTION_PRINT (result); goto _catch; } }
#define _throw(ERROR)                       { result = ERROR; EXCEPTION_PRINT (result); goto _catch; }
#define _throwif(ERROR, COND)               if (M3_UNLIKELY(COND)) { _throw(ERROR); }

#define _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

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

// const char* err2 = error_details(trapStackOverflow, "Errore alla linea %d", line_number);

#endif // m3_exception_h
