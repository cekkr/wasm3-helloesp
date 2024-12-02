
#ifndef m3_pointers_h
#define m3_pointers_h

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
bool safe_free(void** ptr);

///
/// Internal
///

#define     m3_Int_Malloc(NAME, SIZE)                   default_malloc(SIZE)
#define     m3_Int_Realloc(NAME, PTR, NEW, OLD)         default_realloc(PTR, NEW, OLD)
#define     m3_Int_AllocStruct(STRUCT)                  (STRUCT *)default_malloc (sizeof (STRUCT))
#define     m3_Int_AllocArray(STRUCT, NUM)              (STRUCT *)default_malloc (sizeof (STRUCT) * (NUM))
#define     m3_Int_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)default_realloc ((void *)(PTR), sizeof (STRUCT) * (NEW)) // , sizeof (STRUCT) * (OLD)
#define     m3_Int_Free(P)                              do { default_free((void*)(P)); (P) = NULL; } while(0)

#endif  // m3_pointers_h