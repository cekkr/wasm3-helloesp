#pragma once

#include "m3_segmented_memory.h"
#include "m3_exception.h"
#include "m3_core.h"

///
/// Pointer validation
///

typedef enum {
    PTR_CHECK_OK = 0,
    PTR_CHECK_NULL,
    PTR_CHECK_UNALIGNED,
    PTR_CHECK_OUT_OF_BOUNDS
} ptr_check_result_t;

ptr_check_result_t validate_pointer(const void* ptr, size_t expected_size);
bool is_ptr_freeable(void* ptr);
bool safe_free(void* ptr);


bool safe_m3_int_free(void** ptr);
bool safe_free_with_check(void** ptr);
bool is_ptr_valid(const void* ptr);

static inline bool is_address_in_range(uintptr_t addr);
bool ultra_safe_ptr_valid(const void* ptr);

////////////////////////////////////////////////////////////////

typedef struct {
    void* ptr;
    size_t size;
    const char* allocation_point;
} safe_ptr_t;

typedef enum {
    PTR_OK = 0,
    PTR_NULL,
    PTR_UNALIGNED,
    PTR_OUT_OF_BOUNDS,
    PTR_ALREADY_FREED,
    PTR_CORRUPTED,
    PTR_INVALID_BLOCK
} ptr_status_t;

bool ultra_safe_free(void** ptr);
ptr_status_t validate_ptr_for_free(const void* ptr);

///
/// Internal
///

#define     m3_Def_Malloc(SIZE)                         default_malloc(SIZE)
#define     m3_Def_Realloc(PTR, NEW)                    default_realloc(PTR, NEW)
#define     m3_Def_AllocStruct(STRUCT)                  (STRUCT *)default_malloc (sizeof (STRUCT))
#define     m3_Def_AllocArray(STRUCT, NUM)              (STRUCT *)default_malloc (sizeof (STRUCT) * (NUM))
#define     m3_Def_ReallocArray(STRUCT, PTR, NEW)       (STRUCT *)default_realloc (PTR, sizeof (STRUCT) * (NEW)) // , sizeof (STRUCT) * (OLD)
#define     m3_Def_Free(P)                              default_free(P)


/*#define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_AllocStruct_Impl  (#STRUCT, sizeof (STRUCT))
#define     m3_AllocArray(STRUCT, NUM)              (STRUCT *)m3_AllocArray_Impl   (#STRUCT, NUM, sizeof (STRUCT))
#define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_ReallocArray_Impl (#STRUCT, (void *)(PTR), (NEW), (OLD), sizeof (STRUCT))
#define     m3_Free(P)                              do { void* p = (void*)(P);                                  \
                                                        if (p) { fprintf(stderr, PRIts ";heap:FreeMem;;;;%p;\n", m3_GetTimestamp(), p); }     \
                                                        m3_Free_Impl (p); (P) = NULL; } while(0)*/

#define     m3_Malloc(MEM, SIZE)                   m3_malloc(MEM, SIZE)
#define     m3_Realloc(MEM, PTR, NEW)               m3_realloc(MEM, PTR, NEW)
#define     m3_AllocStruct(MEM, STRUCT)                  (STRUCT *)m3_malloc (MEM, sizeof (STRUCT))
#define     m3_AllocArray(MEM, STRUCT, NUM)              (STRUCT *)m3_malloc (MEM, sizeof (STRUCT) * (NUM))
#define     m3_ReallocArray(MEM, PTR, STRUCT, NEW)          (STRUCT *)m3_realloc (MEM, PTR, sizeof (STRUCT) * (NEW))
#define     m3_Free(MEM, PTR)                              m3_free(MEM, PTR)
//#define     m3_FreeMemory(P)                        do { m3_free((void*)(P), true); (P) = NULL; } while(0) 

#define m3_Int_As_Def 1
#if m3_Dyn_As_Def

#define     m3_Dyn_Malloc(SIZE)                         default_malloc(SIZE)
#define     m3_Dyn_Realloc(PTR, NEW)                    default_realloc(PTR, NEW)
#define     m3_Dyn_AllocStruct(STRUCT)                  (STRUCT *)default_malloc (sizeof (STRUCT))
#define     m3_Dyn_AllocArray(STRUCT, NUM)              (STRUCT *)default_malloc (sizeof (STRUCT) * (NUM))
#define     m3_Dyn_ReallocArray(STRUCT, PTR, NEW)       (STRUCT *)default_realloc (PTR, sizeof (STRUCT) * (NEW)) // , sizeof (STRUCT) * (OLD)
#define     m3_Dyn_Free(P)                              default_free(P)

#else 

#define     m3_Dyn_Malloc(MEM, SIZE)                   m3_Malloc(MEM, SIZE)      
#define     m3_Dyn_Realloc(MEM, PTR, NEW)               m3_Realloc(MEM, PTR, NEW)
#define     m3_Dyn_AllocStruct(MEM, STRUCT)                  m3_AllocStruct(MEM, STRUCT)
#define     m3_Dyn_AllocArray(MEM, STRUCT, NUM)              m3_AllocArray(MEM, STRUCT, NUM)
#define     m3_Dyn_ReallocArray(MEM, PTR, STRUCT, NEW)          m3_ReallocArray(MEM, PTR, STRUCT, NEW)
#define     m3_Dyn_Free(MEM, PTR)    

#endif

///
/// Pointer infos
///


typedef struct {
    bool is_valid;
    bool is_in_dram;
    bool is_in_iram;
    bool is_in_executable_range;
    bool is_aligned;
    bool is_in_stack_range;
    const char* region_name;
    uint32_t distance_to_heap_end;
} pointer_info_t;

pointer_info_t analyze_pointer(const void* ptr);
bool print_pointer_report(const void* ptr);
void print_pointer_info(const void* ptr, pointer_info_t info);
