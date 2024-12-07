//
//  m3_api_libc.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ifndef m3_api_libc_h
#define m3_api_libc_h

#include "m3_api_esp_wasi.h"
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

M3Result    m3_LinkLibC     (IM3Module io_module);
M3Result    m3_LinkSpecTest (IM3Module io_module);

d_m3EndExternC

#endif // m3_api_libc_h
