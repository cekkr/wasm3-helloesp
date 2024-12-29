
M3Result Read_u64(IM3Memory memory, u64* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    u8* dest_ptr;
    
    if (memory) {
        source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        dest_ptr = (u8*)m3_ResolvePointer(memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = (u8*)o_value;
    }
    
    const u8* check_ptr = source_ptr + sizeof(u64);
    
    if ((void*)check_ptr > (void*)i_end){
        __read_checkWasmUnderrun(check_ptr, i_end);
        return m3Err_wasmUnderrun;
    }

    memcpy(dest_ptr, source_ptr, sizeof(u64));
    M3_BSWAP_u64(*(u64*)dest_ptr);
    *io_bytes = check_ptr;
    return m3Err_none;
}

M3Result Read_u32(IM3Memory memory, u32* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    u8* dest_ptr;
    
    if (memory) {
        source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        dest_ptr = (u8*)m3_ResolvePointer(memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = (u8*)o_value;
    }
    
    const u8* check_ptr = source_ptr + sizeof(u32);
    
    if ((void*)check_ptr > (void*)i_end){
         __read_checkWasmUnderrun(check_ptr, i_end);
        return m3Err_wasmUnderrun;
    }

    memcpy(dest_ptr, source_ptr, sizeof(u32));
    M3_BSWAP_u32(*(u32*)dest_ptr);
    *io_bytes = check_ptr;
    return m3Err_none;
}

#if d_m3ImplementFloat
M3Result Read_f64(IM3Memory memory, f64* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    u8* dest_ptr;
    
    if (memory) {
        source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        dest_ptr = (u8*)m3_ResolvePointer(memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = (u8*)o_value;
    }
    
    const u8* check_ptr = source_ptr + sizeof(f64);
    
    if ((void*)check_ptr > (void*)i_end){
         __read_checkWasmUnderrun(check_ptr, i_end);
        return m3Err_wasmUnderrun;
    }

    memcpy(dest_ptr, source_ptr, sizeof(f64));
    M3_BSWAP_f64(*(f64*)dest_ptr);
    *io_bytes = check_ptr;
    return m3Err_none;
}

M3Result Read_f32(IM3Memory memory, f32* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    u8* dest_ptr;
    
    if (memory) {
        source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        dest_ptr = (u8*)m3_ResolvePointer(memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = (u8*)o_value;
    }
    
    const u8* check_ptr = source_ptr + sizeof(f32);
    
    if ((void*)check_ptr > (void*)i_end){
         __read_checkWasmUnderrun(check_ptr, i_end);
        return m3Err_wasmUnderrun;
    }

    memcpy(dest_ptr, source_ptr, sizeof(f32));
    M3_BSWAP_f32(*(f32*)dest_ptr);
    *io_bytes = check_ptr;
    return m3Err_none;
}
#endif

M3Result Read_u8(IM3Memory memory, u8* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    u8* dest_ptr;
    
    if (memory) {
        source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        dest_ptr = (u8*)m3_ResolvePointer(memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = (u8*)o_value;
    }
    
    const u8* check_ptr = source_ptr + sizeof(u8);
    
    if ((void*)check_ptr > (void*)i_end){
         __read_checkWasmUnderrun(check_ptr, i_end);
        return m3Err_wasmUnderrun;
    }

    *dest_ptr = *source_ptr;
    *io_bytes = check_ptr;
    return m3Err_none;
}

M3Result Read_opcode(IM3Memory memory, m3opcode_t* o_value, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    m3opcode_t* dest_ptr;
    
    if (memory) {
        source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        dest_ptr = (m3opcode_t*)m3_ResolvePointer(memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = o_value;
    }

    const u8* check_ptr = source_ptr + 1;        
    if ((void*)check_ptr > (void*)i_end){
         __read_checkWasmUnderrun(check_ptr, i_end);
        return m3Err_wasmUnderrun;
    }

    m3opcode_t opcode = *source_ptr++;

#if d_m3CascadedOpcodes == 0
    if (M3_UNLIKELY(opcode == c_waOp_extended)) {
        if ((void*)source_ptr > (void*)i_end){
             __read_checkWasmUnderrun(source_ptr, i_end);
            return m3Err_wasmUnderrun;
        }
        opcode = (opcode << 8) | (*source_ptr++);
    }
#endif

    *dest_ptr = opcode;
    *io_bytes = source_ptr;
    return m3Err_none;
}

M3Result ReadLebUnsigned(IM3Memory memory, u64* o_value, u32 i_maxNumBits, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    u64* dest_ptr;
    
    if (memory) {
        source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        dest_ptr = (u64*)m3_ResolvePointer(memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = o_value;
    }

    u64 value = 0;
    u32 shift = 0;
    const u8* check_ptr = source_ptr;
    M3Result result = m3Err_wasmUnderrun;

    while (check_ptr <= (const u8*)i_end) {
        u64 byte = check_ptr++;
        value |= ((byte & 0x7f) << shift);
        shift += 7;

        if ((byte & 0x80) == 0) {
            result = m3Err_none;
            break;
        }

        if (shift >= i_maxNumBits) {
            result = m3Err_lebOverflow;
            break;
        }
    }

    *dest_ptr = value;
    *io_bytes = check_ptr;

    if(result == m3Err_wasmUnderrun) {
         __read_checkWasmUnderrun(check_ptr, i_end);
    }

    return result;
}

M3Result ReadLebSigned(IM3Memory memory, i64* o_value, u32 i_maxNumBits, bytes_t* io_bytes, cbytes_t i_end) {
    if (!io_bytes || !o_value) 
        return m3Err_malformedData;

    const u8* source_ptr;
    i64* dest_ptr;
    
    if (memory) {
        source_ptr = (const u8*)m3_ResolvePointer(memory, *io_bytes);
        dest_ptr = (i64*)m3_ResolvePointer(memory, o_value);
        if (!source_ptr || source_ptr == ERROR_POINTER || !dest_ptr) 
            return m3Err_malformedData;
    } else {
        if (!*io_bytes) 
            return m3Err_malformedData;
        source_ptr = (const u8*)*io_bytes;
        dest_ptr = o_value;
    }

    i64 value = 0;
    u32 shift = 0;
    const u8* check_ptr = source_ptr;
    M3Result result = m3Err_wasmUnderrun;

    while (check_ptr <= (const u8*)i_end) {
        u64 byte = *check_ptr++;
        value |= ((byte & 0x7f) << shift);
        shift += 7;

        if ((byte & 0x80) == 0) {
            if ((byte & 0x40) && (shift < 64)) {
                u64 extend = 0;
                value |= (~extend << shift);
            }
            result = m3Err_none;
            break;
        }

        if (shift >= i_maxNumBits) {
            result = m3Err_lebOverflow;
            break;
        }
    }    

    *dest_ptr = value;
    *io_bytes = check_ptr;

    if(result == m3Err_wasmUnderrun) {
         __read_checkWasmUnderrun(check_ptr, i_end);
    }

    return result;
}