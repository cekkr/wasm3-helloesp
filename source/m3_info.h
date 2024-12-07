//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#include "m3_api_esp_wasi.h"
#include "m3_api_libc.h"
#include "m3_api_wasi.h"
#include "m3_code.h"
#include "m3_compile.h"
#include "m3_config.h"
#include "m3_core.h"
#include "m3_env.h"
#include "m3_exception.h"
#include "m3_exec.h"
#include "m3_exec_defs.h"
#include "m3_function.h"
#include "m3_includes.h"
#include "m3_math_utils.h"
#include "m3_parse.h"
#include "m3_pointers.h"
#include "m3_segmented_memory.h"
#include "wasm3.h"
#include "wasm3_defs.h"

d_m3BeginExternC

void            ProfileHit              (cstr_t i_operationName);

#ifdef DEBUG

void            dump_type_stack         (IM3Compilation o);
void            log_opcode              (IM3Compilation o, m3opcode_t i_opcode);
const char *    get_indention_string    (IM3Compilation o);
void            log_emit                (IM3Compilation o, IM3Operation i_operation);

cstr_t          SPrintFuncTypeSignature (IM3FuncType i_funcType);

#else // DEBUG

#define         dump_type_stack(...)      {}
#define         log_opcode(...)           {}
#define         get_indention_string(...) ""
#define         emit_stack_dump(...)      {}
#define         log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
