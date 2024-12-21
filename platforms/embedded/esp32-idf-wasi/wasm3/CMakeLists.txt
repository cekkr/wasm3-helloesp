#include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#project(wasm3)

file(GLOB_RECURSE M3_SOURCES "wasm3/*.c")

# Rimuovi main.cpp dai sorgenti
idf_component_register(
                    SRCS ${M3_SOURCES} #"m3_api_esp_wasi.c"
                    INCLUDE_DIRS "wasm3" "${IDF_PATH}/components/esp_system/include"
                    REQUIRES esp_system esp_mm
                )

idf_build_set_property(COMPILE_OPTIONS "-Wno-error=implicit-function-declaration" APPEND)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -O3 -DESP32 -Dd_m3MaxFunctionStackHeight=256)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=return-type -Wno-error=unused-but-set-parameter 
                        -Wno-error=unused-value -Wno-error=maybe-uninitialized -Wno-error=uninitialized -Wno-error=format
                        -Wno-error=multistatement-macros -Wno-error=implicit-int  
                    )

# Avoid IRAM
target_compile_options(${COMPONENT_LIB} PRIVATE 
    -fno-jump-tables 
    -fno-tree-switch-conversion
)
