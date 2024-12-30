#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "esp_log.h"

#define NOINLINE_ATTR __attribute__((noinline))

#if WASM_ENABLE_OP_TRACE

#define TRACE_STACK_DEPTH_MAX 128
#define TRACE_STACK_REPEAT 1

typedef struct {
    const void* op;
    int entry_count;
    int exit_count;
    const char* func_name;
} trace_entry_t;

static struct {
    trace_entry_t entries[TRACE_STACK_DEPTH_MAX];
    int current_stack_depth;
    const char* TAG;
} trace_context = {
    .current_stack_depth = 0,
    .TAG = "WASM3"
};

// Funzioni di tracciamento unificate
NOINLINE_ATTR static void trace_enter(const void* op, int depth, const char* func_name) { 
    if (depth >= TRACE_STACK_DEPTH_MAX) {
        ESP_LOGE(trace_context.TAG, "Stack overflow! Max depth: %d", TRACE_STACK_DEPTH_MAX);
        return;
    }

#if TRACE_STACK_REPEAT
    for (int i = 0; i < trace_context.current_stack_depth; i++) {
        if (trace_context.entries[i].op == op) {
            trace_context.entries[i].entry_count++;
            ESP_LOGW(trace_context.TAG, "REPEAT ENTER [%d]: %s (%p) - Count: %d", 
                     depth, func_name ? func_name : "unknown", op, trace_context.entries[i].entry_count);
            return;
        }
    }
#endif

    trace_entry_t* new_entry = &trace_context.entries[trace_context.current_stack_depth];
    new_entry->op = op;
    new_entry->entry_count = 1;
    new_entry->exit_count = 0;
    new_entry->func_name = func_name;
    
    ESP_LOGD(trace_context.TAG, "ENTER [%d]: %s (%p)", depth, func_name ? func_name : "unknown", op);
    waitForIt();
}

NOINLINE_ATTR static void trace_exit(const void* op, int depth, const char* func_name) {
    if (depth < 0) {
        ESP_LOGE(trace_context.TAG, "Stack underflow!");
        return;
    }

#if TRACE_STACK_REPEAT
    for (int i = 0; i < trace_context.current_stack_depth; i++) {
        if (trace_context.entries[i].op == op) {
            trace_context.entries[i].exit_count++;
            ESP_LOGD(trace_context.TAG, "EXIT [%d]: %s (%p) - Enter/Exit: %d/%d", 
                     depth, func_name ? func_name : "unknown", op, 
                     trace_context.entries[i].entry_count, trace_context.entries[i].exit_count);
            return;
        }
    }
#endif

    ESP_LOGD(trace_context.TAG, "EXIT [%d]: %s (%p)", depth, func_name ? func_name : "unknown", op);
}

// Macro di utilità per il debug (unused)
//#define TRACE_ENTER() trace_enter(__func__, trace_context.current_stack_depth, __func__)
//#define TRACE_EXIT()  trace_exit(__func__, trace_context.current_stack_depth, __func__)

#endif

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#if WASM_ENABLE_CHECK_MEMORY_PTR
//#define CHECK_MEMORY_PTR(mem) ESP_LOGI("WASM3", "Current memory ptr: %p", mem); LOG_FLUSH; esp_backtrace_print(1)
#define CHECK_MEMORY_PTR(mem, pos) ESP_LOGW("WASM3", "Current memory ptr: %p (in %s)", mem, pos)
#else 
#define CHECK_MEMORY_PTR(mem, pos) nothing_todo()
#endif

