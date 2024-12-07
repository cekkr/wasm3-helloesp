//
//  m3_api_wasi.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ifndef m3_api_wasi_h
#define m3_api_wasi_h

#include "m3_api_esp_wasi.h"
#include "m3_api_libc.h"
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

#if defined(d_m3HasUVWASI)
#include "uvwasi.h"
#endif


d_m3BeginExternC

typedef struct m3_wasi_context_t
{
    i32                     exit_code;
    u32                     argc;
    ccstr_t *               argv;
} m3_wasi_context_t;

M3Result    m3_LinkWASI             (IM3Module io_module);

#if defined(d_m3HasUVWASI)

M3Result    m3_LinkWASIWithOptions  (IM3Module io_module, uvwasi_options_t uvwasiOptions);

#endif

m3_wasi_context_t* m3_GetWasiContext();

d_m3EndExternC

#endif // m3_api_wasi_h
