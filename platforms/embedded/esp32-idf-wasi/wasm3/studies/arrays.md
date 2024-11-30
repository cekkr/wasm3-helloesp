M3MemoryInfo

In wasm3, la dimensione di un array viene tipicamente letta utilizzando funzioni di decodifica LEB128 (Little Endian Base 128), che è il formato utilizzato da WASM per codificare i numeri interi di lunghezza variabile. La funzione principale è `ReadLEB_u32`:

```c
M3Result ReadLEB_u32 (u32 * o_value, bytes_t * io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;
    
    u32 value = 0;
    u32 shift = 0;
    
    while (true) 
    {
        if (*io_bytes >= i_end)
            return m3Err_malformedBytecode;
            
        u8 byte = ** io_bytes;
        (*io_bytes)++;
        
        value |= ((byte & 0x7f) << shift);
        shift += 7;
        
        if ((byte & 0x80) == 0)
            break;
            
        if (shift >= 32)
            return m3Err_malformedBytecode;
    }
    
    * o_value = value;
    return result;
}
```

Questa funzione:
1. Legge byte per byte finché non trova un byte con il bit più significativo a 0
2. Ogni byte fornisce 7 bit di dati
3. Ricostruisce il numero intero finale che rappresenta la lunghezza dell'array

La dimensione viene usata in questo modo tipicamente:
```c
u32 arrayLength;
result = ReadLEB_u32(&arrayLength, io_bytes, i_end);
if (result) return result;

// Ora arrayLength contiene la dimensione dell'array
```

Non c'è una funzione tipo `sizeof()` perché wasm3 lavora principalmente con stream di bytecode dove le dimensioni sono codificate esplicitamente nel formato LEB128.



Ecco un esempio di come creare e rappresentare un array nel bytecode WASM:

```wat
;; Definizione di un array di 5 interi a 32 bit
(module
  ;; Allocazione memoria (1 pagina = 64KB)
  (memory 1)
  
  ;; Inizializzazione array con dati
  (data (i32.const 0) "\01\00\00\00"  ;; elemento 1
                      "\02\00\00\00"  ;; elemento 2
                      "\03\00\00\00"  ;; elemento 3
                      "\04\00\00\00"  ;; elemento 4
                      "\05\00\00\00") ;; elemento 5

  ;; Funzione per ottenere elemento dell'array
  (func $getElement (param $index i32) (result i32)
    ;; Calcola offset (index * 4 bytes per i32)
    local.get $index
    i32.const 4
    i32.mul
    ;; Carica valore dalla memoria
    i32.load
  )

  ;; Funzione per impostare elemento dell'array
  (func $setElement (param $index i32) (param $value i32)
    ;; Calcola offset
    local.get $index
    i32.const 4
    i32.mul
    ;; Memorizza il valore
    local.get $value
    i32.store
  )

  ;; Esporta le funzioni
  (export "getElement" (func $getElement))
  (export "setElement" (func $setElement))
)

```

Quando questo viene compilato in bytecode binario, appare così:

```c
// Header WASM
0x00, 0x61, 0x73, 0x6D,  // magic number (\0asm)
0x01, 0x00, 0x00, 0x00,  // version 1

// Sezione Memory
0x05,                    // section code (Memory)
0x03,                    // section size
0x01,                    // number of memories
0x00,                    // minimum 1 page
0x01,                    // maximum 1 page

// Sezione Data (inizializzazione array)
0x0B,                    // section code (Data)
0x1D,                    // section size
0x01,                    // number of data segments
0x00,                    // memory index
0x41, 0x00,             // i32.const 0 (offset)
0x0B,                    // end
0x14,                    // data size (20 bytes)
// array data [1,2,3,4,5]
0x01, 0x00, 0x00, 0x00,  // 1
0x02, 0x00, 0x00, 0x00,  // 2
0x03, 0x00, 0x00, 0x00,  // 3
0x04, 0x00, 0x00, 0x00,  // 4
0x05, 0x00, 0x00, 0x00   // 5

```

In wasm3, questo bytecode viene gestito così:

1. **Lettura della memoria**:
```c
// Legge sezione memoria
if (*(u32*)bytes == 0x05) {  // Memory section
    bytes += 4;
    u32 pages = ReadLEB_u32(&pages, &bytes, end);
    InitMemory(runtime, pages);
}
```

2. **Lettura dei dati**:
```c
// Legge sezione data
if (*(u32*)bytes == 0x0B) {  // Data section
    bytes += 4;
    u32 size = ReadLEB_u32(&size, &bytes, end);
    u32 offset = ReadLEB_u32(&offset, &bytes, end);
    
    // Copia i dati nella memoria
    memcpy(runtime->memory.mallocated + offset, bytes, size);
    bytes += size;
}
```

Aspetti chiave da notare:

1. L'array è memorizzato in memoria lineare WASM
2. Ogni elemento i32 occupa 4 bytes (little-endian)
3. L'accesso è basato su offset (index * sizeof(element))
4. La memoria è inizializzata nella sezione data
5. Le funzioni di accesso fanno controlli di bounds

Per accedere all'array in C tramite wasm3:
```c
// Lettura elemento
u32 GetArrayElement(IM3Runtime runtime, u32 index) {
    if (index * 4 >= runtime->memory.numPages * 65536) 
        return m3Err_trapOutOfBoundsMemoryAccess;
    
    return *(u32*)(runtime->memory.mallocated + (index * 4));
}
```

Vuoi che approfondiamo qualche aspetto specifico della gestione degli array in WASM o della loro implementazione in wasm3?