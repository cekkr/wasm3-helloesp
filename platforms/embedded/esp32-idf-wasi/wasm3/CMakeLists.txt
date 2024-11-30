#include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#project(wasm3)

file(GLOB_RECURSE M3_SOURCES "wasm3/*.c")

# Rimuovi main.cpp dai sorgenti
idf_component_register(SRCS ${M3_SOURCES} "m3_api_esp_wasi.c"
                    INCLUDE_DIRS "wasm3")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -O3 -DESP32 -Dd_m3MaxFunctionStackHeight=256)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers)