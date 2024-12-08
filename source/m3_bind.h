//
//  m3_bind.h
//
//  Created by Steven Massey on 2/27/20.
//  Copyright © 2020 Steven Massey. All rights reserved.
//

#pragma once

#include "m3_env.h"
#include "m3_exception.h"

d_m3BeginExternC

u8          ConvertTypeCharToTypeId     (char i_code);
M3Result    SignatureToFuncType         (IM3FuncType * o_functionType, ccstr_t i_signature);

d_m3EndExternC

