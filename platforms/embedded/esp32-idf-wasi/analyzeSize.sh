#!/bin/bash

#export IDF_PATH="/Users/riccardo/Sources/Libs/esp-idf-v5.3.1"
#alias python=python3

CUR_DIR=$(pwd)

cd "$IDF_PATH"
# Salva la directory corrente di IDF_PATH
IDF_PWD=$(pwd)  

. ./export.sh

# Torna alla directory di IDF_PATH (potrebbe essere stata modificata da export.sh)
cd "$IDF_PWD" 
cd "$CUR_DIR"

xtensa-esp32-elf-nm -S --size-sort build/wasm3.elf | grep -i iram

xtensa-esp32-elf-objdump -d build/wasm3.elf | grep -A 20 "m3_compile"