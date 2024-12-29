#include "m3_debug_post.h"
#include "m3_compile.h"

void WASM3_Debug_PrintOpsInfo(){
    ESP_LOGW("WASM3","WASM3_Debug_PrintOpsInfo");
    ESP_LOGW("WASM3", "Compile_Return: %p", &Compile_Return);
}