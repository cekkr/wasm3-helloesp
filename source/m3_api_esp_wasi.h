//
//  m3_api_esp_wasi.h
//
//  Created by Volodymyr Shymanskyy on 01/07/20.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#pragma once

#include "wasm3.h"
#include "m3_function.h"
#include "m3_exception.h"

d_m3BeginExternC

typedef struct m3_wasi_context_t
{
    i32                     exit_code;
    u32                     argc;
    ccstr_t *               argv;
} m3_wasi_context_t;

M3Result    m3_LinkEspWASI     (IM3Module io_module);
M3Result m3_LinkEspWASI_Hello(IM3Module module);

m3_wasi_context_t* m3_GetWasiContext();

d_m3EndExternC
