//
//  m3_bind.h
//
//  Created by Steven Massey on 2/27/20.
//  Copyright © 2020 Steven Massey. All rights reserved.
//

#ifndef m3_bind_h
#define m3_bind_h

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
#include "m3_info.h"
#include "m3_math_utils.h"
#include "m3_parse.h"
#include "m3_pointers.h"
#include "m3_segmented_memory.h"
#include "wasm3.h"
#include "wasm3_defs.h"

d_m3BeginExternC

u8          ConvertTypeCharToTypeId     (char i_code);
M3Result    SignatureToFuncType         (IM3FuncType * o_functionType, ccstr_t i_signature);

d_m3EndExternC

#endif /* m3_bind_h */
