#include "m3_debug_post.h"
#include "m3_compile.h"

#if WASM_ENABLE_OP_TRACE
void WASM3_Debug_PrintOpsInfo(){
    ESP_LOGW("WASM3","WASM3_Debug_PrintOpsInfo");
    ESP_LOGW("WASM3", "Compile_Return: %p", &Compile_Return); 
    ESP_LOGW("WASM3", "Compile_Const_i32: %p", &Compile_Const_i32); 
}
#endif