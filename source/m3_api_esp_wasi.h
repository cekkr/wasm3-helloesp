//
//  m3_api_esp_wasi.h
//
//  Created by Volodymyr Shymanskyy on 01/07/20.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#pragma once

#include "wasm3.h"
#include "m3_exception.h"

#if PASSTHROUGH_HELLOESP
#include "he_cmd.h"
#endif

d_m3BeginExternC

typedef struct m3_wasi_context_t
{
    i32                     exit_code;
    u32                     argc;
    ccstr_t *               argv;

    #if PASSTHROUGH_HELLOESP
    shell_t * shell;
    #endif
} m3_wasi_context_t;

// Structure describing the native function to register
typedef struct {
    const char* name;       // Function's name
    void* func;     // Function's pointer
    const char* signature;  // Function's signature
} WasmFunctionEntry;

M3Result    m3_LinkEspWASI     (IM3Module io_module);

#if PASSTHROUGH_HELLOESP

M3Result m3_LinkEspWASI_Hello(IM3Module module, shell_t *shell, m3_wasi_context_t** ctx);
#endif

m3_wasi_context_t* m3_GetWasiContext();

d_m3EndExternC
