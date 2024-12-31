//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#pragma once

#include "m3_compile.h"
#include "m3_exception.h"

d_m3BeginExternC

void            ProfileHit              (OP_TRACE_TYPE i_operationName);

#if DEBUG

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
