//  m3_helpers.h
//
//  Created by Riccardo Cecchini on 12/12/24.
//  Copyright © 2019 Riccardo Cecchini. All rights reserved.

#pragma once

#include "wasm3.h"

static inline size_t min3(size_t a, size_t b, size_t c)
{
    return min(min(a, b), c);
}