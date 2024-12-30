//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#pragma once

#include <stdarg.h>
#include "esp_debug_helpers.h"
#include "esp_private/panic_internal.h"

//#include "m3_config.h"
//#include "m3_config_platforms.h"

#if WASM_DEBUG || DEBUG
#undef d_m3EnableExceptionBreakpoint
#define d_m3EnableExceptionBreakpoint 1
#endif

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

char* error_details(const char* base_error, const char* format, ...);

// const char* err2 = error_details(trapStackOverflow, "Errore alla linea %d", line_number);

void custom_panic_handler(void* frame, panic_info_t* info);

void print_last_two_callers(); //todo: it doens't work: deprecate it
void nothing_todo();