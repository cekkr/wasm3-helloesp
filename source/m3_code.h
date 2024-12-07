//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_code_h
#define m3_code_h

//d_m3BeginExternC

#include "m3_api_esp_wasi.h"
#include "m3_api_libc.h"
#include "m3_api_wasi.h"
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

struct M3CodeMappingPage;

typedef struct M3CodePageHeader
{
    struct M3CodePage *           next;

    u32                           lineIndex;
    u32                           numLines;
    u32                           sequence;       // this is just used for debugging; could be removed
    u32                           usageCount;

# if d_m3RecordBacktraces
    struct M3CodeMappingPage *    mapping;
# endif // d_m3RecordBacktraces
}
M3CodePageHeader;

typedef struct M3CodePage
{
    M3CodePageHeader        info;
    code_t                  code                [1];
}
M3CodePage;

typedef M3CodePage *    IM3CodePage;


IM3CodePage             NewCodePage             (IM3Runtime i_runtime, u32 i_minNumLines);

void                    FreeCodePages           (IM3CodePage * io_list);

u32                     NumFreeLines            (IM3CodePage i_page);
pc_t                    GetPageStartPC          (IM3CodePage i_page);
pc_t                    GetPagePC               (IM3CodePage i_page);
void                    EmitWord_impl           (IM3CodePage i_page, void* i_word);
void                    EmitWord32              (IM3CodePage i_page, u32 i_word);
void                    EmitWord64              (IM3CodePage i_page, u64 i_word);
# if d_m3RecordBacktraces
void                    EmitMappingEntry        (IM3CodePage i_page, u32 i_moduleOffset);
# endif // d_m3RecordBacktraces

void                    PushCodePage            (IM3CodePage * io_list, IM3CodePage i_codePage);
IM3CodePage             PopCodePage             (IM3CodePage * io_list);

IM3CodePage             GetEndCodePage          (IM3CodePage i_list); // i_list = NULL is valid
u32                     CountCodePages          (IM3CodePage i_list); // i_list = NULL is valid

# if d_m3RecordBacktraces
bool                    ContainsPC              (IM3CodePage i_page, pc_t i_pc);
bool                    MapPCToOffset           (IM3CodePage i_page, pc_t i_pc, u32 * o_moduleOffset);
# endif // d_m3RecordBacktraces

# ifdef DEBUG
void                    dump_code_page            (IM3CodePage i_codePage, pc_t i_startPC);
# endif

#define EmitWord(page, val) EmitWord_impl(page, (void*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

# if d_m3RecordBacktraces

typedef struct M3CodeMapEntry
{
    u32          pcOffset;
    u32          moduleOffset;
}
M3CodeMapEntry;

typedef struct M3CodeMappingPage
{
    pc_t              basePC;
    u32               size;
    u32               capacity;
    M3CodeMapEntry    entries     [];
}
M3CodeMappingPage;

# endif // d_m3RecordBacktraces

//d_m3EndExternC

#endif // m3_code_h
