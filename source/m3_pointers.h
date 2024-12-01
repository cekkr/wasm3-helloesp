
#ifndef m3_pointers_h
#define m3_pointers_h

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

#endif  // m3_pointers_h