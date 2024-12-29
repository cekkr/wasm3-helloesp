/// Unused files, just for reference for generate_wasm_ops.py
/// operations took from m3_compile.c

const M3OpInfo c_operations [] =
{
    M3OP( "unreachable",         0, none,   d_logOp (Unreachable),              Compile_Unreachable ),  // 0x00
    M3OP( "nop",                 0, none,   d_emptyOpList,                      Compile_Nop ),          // 0x01 .
    M3OP( "block",               0, none,   d_emptyOpList,                      Compile_LoopOrBlock ),  // 0x02
    M3OP( "loop",                0, none,   d_logOp (Loop),                     Compile_LoopOrBlock ),  // 0x03
    M3OP( "if",                 -1, none,   d_emptyOpList,                      Compile_If ),           // 0x04
    M3OP( "else",                0, none,   d_emptyOpList,                      Compile_Nop ),          // 0x05

    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                          // 0x06...0x0a

    M3OP( "end",                 0, none,   d_emptyOpList,                      Compile_End ),          // 0x0b
    M3OP( "br",                  0, none,   d_logOp (Branch),                   Compile_Branch ),       // 0x0c
    M3OP( "br_if",              -1, none,   d_logOp2 (BranchIf_r, BranchIf_s),  Compile_Branch ),       // 0x0d
    M3OP( "br_table",           -1, none,   d_logOp (BranchTable),              Compile_BranchTable ),  // 0x0e
    M3OP( "return",              0, any,    d_logOp (Return),                   Compile_Return ),       // 0x0f
    M3OP( "call",                0, any,    d_logOp (Call),                     Compile_Call ),         // 0x10
    M3OP( "call_indirect",       0, any,    d_logOp (CallIndirect),             Compile_CallIndirect ), // 0x11
    M3OP( "return_call",         0, any,    d_emptyOpList,                      Compile_Call ),         // 0x12 TODO: Optimize
    M3OP( "return_call_indirect",0, any,    d_emptyOpList,                      Compile_CallIndirect ), // 0x13

    M3OP_RESERVED,  M3OP_RESERVED,                                                                      // 0x14...
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                        // ...0x19

    M3OP( "drop",               -1, none,   d_emptyOpList,                      Compile_Drop ),         // 0x1a
    M3OP( "select",             -2, any,    d_emptyOpList,                      Compile_Select  ),      // 0x1b

    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                        // 0x1c...0x1f

    M3OP( "local.get",          1,  any,    d_emptyOpList,                      Compile_GetLocal ),     // 0x20
    M3OP( "local.set",          1,  none,   d_emptyOpList,                      Compile_SetLocal ),     // 0x21
    M3OP( "local.tee",          0,  any,    d_emptyOpList,                      Compile_SetLocal ),     // 0x22
    M3OP( "global.get",         1,  none,   d_emptyOpList,                      Compile_GetSetGlobal ), // 0x23
    M3OP( "global.set",         1,  none,   d_emptyOpList,                      Compile_GetSetGlobal ), // 0x24

    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                       // 0x25...0x27

    M3OP( "i32.load",           0,  i_32,   d_unaryOpList (i32, Load_i32),      Compile_Load_Store ),   // 0x28
    M3OP( "i64.load",           0,  i_64,   d_unaryOpList (i64, Load_i64),      Compile_Load_Store ),   // 0x29
    M3OP_F( "f32.load",         0,  f_32,   d_unaryOpList (f32, Load_f32),      Compile_Load_Store ),   // 0x2a
    M3OP_F( "f64.load",         0,  f_64,   d_unaryOpList (f64, Load_f64),      Compile_Load_Store ),   // 0x2b

    M3OP( "i32.load8_s",        0,  i_32,   d_unaryOpList (i32, Load_i8),       Compile_Load_Store ),   // 0x2c
    M3OP( "i32.load8_u",        0,  i_32,   d_unaryOpList (i32, Load_u8),       Compile_Load_Store ),   // 0x2d
    M3OP( "i32.load16_s",       0,  i_32,   d_unaryOpList (i32, Load_i16),      Compile_Load_Store ),   // 0x2e
    M3OP( "i32.load16_u",       0,  i_32,   d_unaryOpList (i32, Load_u16),      Compile_Load_Store ),   // 0x2f

    M3OP( "i64.load8_s",        0,  i_64,   d_unaryOpList (i64, Load_i8),       Compile_Load_Store ),   // 0x30
    M3OP( "i64.load8_u",        0,  i_64,   d_unaryOpList (i64, Load_u8),       Compile_Load_Store ),   // 0x31
    M3OP( "i64.load16_s",       0,  i_64,   d_unaryOpList (i64, Load_i16),      Compile_Load_Store ),   // 0x32
    M3OP( "i64.load16_u",       0,  i_64,   d_unaryOpList (i64, Load_u16),      Compile_Load_Store ),   // 0x33
    M3OP( "i64.load32_s",       0,  i_64,   d_unaryOpList (i64, Load_i32),      Compile_Load_Store ),   // 0x34
    M3OP( "i64.load32_u",       0,  i_64,   d_unaryOpList (i64, Load_u32),      Compile_Load_Store ),   // 0x35

    M3OP( "i32.store",          -2, none,   d_binOpList (i32, Store_i32),       Compile_Load_Store ),   // 0x36
    M3OP( "i64.store",          -2, none,   d_binOpList (i64, Store_i64),       Compile_Load_Store ),   // 0x37
    M3OP_F( "f32.store",        -2, none,   d_storeFpOpList (f32, Store_f32),   Compile_Load_Store ),   // 0x38
    M3OP_F( "f64.store",        -2, none,   d_storeFpOpList (f64, Store_f64),   Compile_Load_Store ),   // 0x39

    M3OP( "i32.store8",         -2, none,   d_binOpList (i32, Store_u8),        Compile_Load_Store ),   // 0x3a
    M3OP( "i32.store16",        -2, none,   d_binOpList (i32, Store_i16),       Compile_Load_Store ),   // 0x3b

    M3OP( "i64.store8",         -2, none,   d_binOpList (i64, Store_u8),        Compile_Load_Store ),   // 0x3c
    M3OP( "i64.store16",        -2, none,   d_binOpList (i64, Store_i16),       Compile_Load_Store ),   // 0x3d
    M3OP( "i64.store32",        -2, none,   d_binOpList (i64, Store_i32),       Compile_Load_Store ),   // 0x3e

    M3OP( "memory.size",        1,  i_32,   d_logOp (MemSize),                  Compile_Memory_Size ),  // 0x3f
    M3OP( "memory.grow",        1,  i_32,   d_logOp (MemGrow),                  Compile_Memory_Grow ),  // 0x40

    M3OP( "i32.const",          1,  i_32,   d_logOp (Const32),                  Compile_Const_i32 ),    // 0x41
    M3OP( "i64.const",          1,  i_64,   d_logOp (Const64),                  Compile_Const_i64 ),    // 0x42
    M3OP_F( "f32.const",        1,  f_32,   d_emptyOpList,                      Compile_Const_f32 ),    // 0x43
    M3OP_F( "f64.const",        1,  f_64,   d_emptyOpList,                      Compile_Const_f64 ),    // 0x44

    M3OP( "i32.eqz",            0,  i_32,   d_unaryOpList (i32, EqualToZero)        , NULL  ),          // 0x45
    M3OP( "i32.eq",             -1, i_32,   d_commutativeBinOpList (i32, Equal)     , NULL  ),          // 0x46
    M3OP( "i32.ne",             -1, i_32,   d_commutativeBinOpList (i32, NotEqual)  , NULL  ),          // 0x47
    M3OP( "i32.lt_s",           -1, i_32,   d_binOpList (i32, LessThan)             , NULL  ),          // 0x48
    M3OP( "i32.lt_u",           -1, i_32,   d_binOpList (u32, LessThan)             , NULL  ),          // 0x49
    M3OP( "i32.gt_s",           -1, i_32,   d_binOpList (i32, GreaterThan)          , NULL  ),          // 0x4a
    M3OP( "i32.gt_u",           -1, i_32,   d_binOpList (u32, GreaterThan)          , NULL  ),          // 0x4b
    M3OP( "i32.le_s",           -1, i_32,   d_binOpList (i32, LessThanOrEqual)      , NULL  ),          // 0x4c
    M3OP( "i32.le_u",           -1, i_32,   d_binOpList (u32, LessThanOrEqual)      , NULL  ),          // 0x4d
    M3OP( "i32.ge_s",           -1, i_32,   d_binOpList (i32, GreaterThanOrEqual)   , NULL  ),          // 0x4e
    M3OP( "i32.ge_u",           -1, i_32,   d_binOpList (u32, GreaterThanOrEqual)   , NULL  ),          // 0x4f

    M3OP( "i64.eqz",            0,  i_32,   d_unaryOpList (i64, EqualToZero)        , NULL  ),          // 0x50
    M3OP( "i64.eq",             -1, i_32,   d_commutativeBinOpList (i64, Equal)     , NULL  ),          // 0x51
    M3OP( "i64.ne",             -1, i_32,   d_commutativeBinOpList (i64, NotEqual)  , NULL  ),          // 0x52
    M3OP( "i64.lt_s",           -1, i_32,   d_binOpList (i64, LessThan)             , NULL  ),          // 0x53
    M3OP( "i64.lt_u",           -1, i_32,   d_binOpList (u64, LessThan)             , NULL  ),          // 0x54
    M3OP( "i64.gt_s",           -1, i_32,   d_binOpList (i64, GreaterThan)          , NULL  ),          // 0x55
    M3OP( "i64.gt_u",           -1, i_32,   d_binOpList (u64, GreaterThan)          , NULL  ),          // 0x56
    M3OP( "i64.le_s",           -1, i_32,   d_binOpList (i64, LessThanOrEqual)      , NULL  ),          // 0x57
    M3OP( "i64.le_u",           -1, i_32,   d_binOpList (u64, LessThanOrEqual)      , NULL  ),          // 0x58
    M3OP( "i64.ge_s",           -1, i_32,   d_binOpList (i64, GreaterThanOrEqual)   , NULL  ),          // 0x59
    M3OP( "i64.ge_u",           -1, i_32,   d_binOpList (u64, GreaterThanOrEqual)   , NULL  ),          // 0x5a

    M3OP_F( "f32.eq",           -1, i_32,   d_commutativeBinOpList (f32, Equal)     , NULL  ),          // 0x5b
    M3OP_F( "f32.ne",           -1, i_32,   d_commutativeBinOpList (f32, NotEqual)  , NULL  ),          // 0x5c
    M3OP_F( "f32.lt",           -1, i_32,   d_binOpList (f32, LessThan)             , NULL  ),          // 0x5d
    M3OP_F( "f32.gt",           -1, i_32,   d_binOpList (f32, GreaterThan)          , NULL  ),          // 0x5e
    M3OP_F( "f32.le",           -1, i_32,   d_binOpList (f32, LessThanOrEqual)      , NULL  ),          // 0x5f
    M3OP_F( "f32.ge",           -1, i_32,   d_binOpList (f32, GreaterThanOrEqual)   , NULL  ),          // 0x60

    M3OP_F( "f64.eq",           -1, i_32,   d_commutativeBinOpList (f64, Equal)     , NULL  ),          // 0x61
    M3OP_F( "f64.ne",           -1, i_32,   d_commutativeBinOpList (f64, NotEqual)  , NULL  ),          // 0x62
    M3OP_F( "f64.lt",           -1, i_32,   d_binOpList (f64, LessThan)             , NULL  ),          // 0x63
    M3OP_F( "f64.gt",           -1, i_32,   d_binOpList (f64, GreaterThan)          , NULL  ),          // 0x64
    M3OP_F( "f64.le",           -1, i_32,   d_binOpList (f64, LessThanOrEqual)      , NULL  ),          // 0x65
    M3OP_F( "f64.ge",           -1, i_32,   d_binOpList (f64, GreaterThanOrEqual)   , NULL  ),          // 0x66

    M3OP( "i32.clz",            0,  i_32,   d_unaryOpList (u32, Clz)                , NULL  ),          // 0x67
    M3OP( "i32.ctz",            0,  i_32,   d_unaryOpList (u32, Ctz)                , NULL  ),          // 0x68
    M3OP( "i32.popcnt",         0,  i_32,   d_unaryOpList (u32, Popcnt)             , NULL  ),          // 0x69

    M3OP( "i32.add",            -1, i_32,   d_commutativeBinOpList (i32, Add)       , NULL  ),          // 0x6a
    M3OP( "i32.sub",            -1, i_32,   d_binOpList (i32, Subtract)             , NULL  ),          // 0x6b
    M3OP( "i32.mul",            -1, i_32,   d_commutativeBinOpList (i32, Multiply)  , NULL  ),          // 0x6c
    M3OP( "i32.div_s",          -1, i_32,   d_binOpList (i32, Divide)               , NULL  ),          // 0x6d
    M3OP( "i32.div_u",          -1, i_32,   d_binOpList (u32, Divide)               , NULL  ),          // 0x6e
    M3OP( "i32.rem_s",          -1, i_32,   d_binOpList (i32, Remainder)            , NULL  ),          // 0x6f
    M3OP( "i32.rem_u",          -1, i_32,   d_binOpList (u32, Remainder)            , NULL  ),          // 0x70
    M3OP( "i32.and",            -1, i_32,   d_commutativeBinOpList (u32, And)       , NULL  ),          // 0x71
    M3OP( "i32.or",             -1, i_32,   d_commutativeBinOpList (u32, Or)        , NULL  ),          // 0x72
    M3OP( "i32.xor",            -1, i_32,   d_commutativeBinOpList (u32, Xor)       , NULL  ),          // 0x73
    M3OP( "i32.shl",            -1, i_32,   d_binOpList (u32, ShiftLeft)            , NULL  ),          // 0x74
    M3OP( "i32.shr_s",          -1, i_32,   d_binOpList (i32, ShiftRight)           , NULL  ),          // 0x75
    M3OP( "i32.shr_u",          -1, i_32,   d_binOpList (u32, ShiftRight)           , NULL  ),          // 0x76
    M3OP( "i32.rotl",           -1, i_32,   d_binOpList (u32, Rotl)                 , NULL  ),          // 0x77
    M3OP( "i32.rotr",           -1, i_32,   d_binOpList (u32, Rotr)                 , NULL  ),          // 0x78

    M3OP( "i64.clz",            0,  i_64,   d_unaryOpList (u64, Clz)                , NULL  ),          // 0x79
    M3OP( "i64.ctz",            0,  i_64,   d_unaryOpList (u64, Ctz)                , NULL  ),          // 0x7a
    M3OP( "i64.popcnt",         0,  i_64,   d_unaryOpList (u64, Popcnt)             , NULL  ),          // 0x7b

    M3OP( "i64.add",            -1, i_64,   d_commutativeBinOpList (i64, Add)       , NULL  ),          // 0x7c
    M3OP( "i64.sub",            -1, i_64,   d_binOpList (i64, Subtract)             , NULL  ),          // 0x7d
    M3OP( "i64.mul",            -1, i_64,   d_commutativeBinOpList (i64, Multiply)  , NULL  ),          // 0x7e
    M3OP( "i64.div_s",          -1, i_64,   d_binOpList (i64, Divide)               , NULL  ),          // 0x7f
    M3OP( "i64.div_u",          -1, i_64,   d_binOpList (u64, Divide)               , NULL  ),          // 0x80
    M3OP( "i64.rem_s",          -1, i_64,   d_binOpList (i64, Remainder)            , NULL  ),          // 0x81
    M3OP( "i64.rem_u",          -1, i_64,   d_binOpList (u64, Remainder)            , NULL  ),          // 0x82
    M3OP( "i64.and",            -1, i_64,   d_commutativeBinOpList (u64, And)       , NULL  ),          // 0x83
    M3OP( "i64.or",             -1, i_64,   d_commutativeBinOpList (u64, Or)        , NULL  ),          // 0x84
    M3OP( "i64.xor",            -1, i_64,   d_commutativeBinOpList (u64, Xor)       , NULL  ),          // 0x85
    M3OP( "i64.shl",            -1, i_64,   d_binOpList (u64, ShiftLeft)            , NULL  ),          // 0x86
    M3OP( "i64.shr_s",          -1, i_64,   d_binOpList (i64, ShiftRight)           , NULL  ),          // 0x87
    M3OP( "i64.shr_u",          -1, i_64,   d_binOpList (u64, ShiftRight)           , NULL  ),          // 0x88
    M3OP( "i64.rotl",           -1, i_64,   d_binOpList (u64, Rotl)                 , NULL  ),          // 0x89
    M3OP( "i64.rotr",           -1, i_64,   d_binOpList (u64, Rotr)                 , NULL  ),          // 0x8a

    M3OP_F( "f32.abs",          0,  f_32,   d_unaryOpList(f32, Abs)                 , NULL  ),          // 0x8b
    M3OP_F( "f32.neg",          0,  f_32,   d_unaryOpList(f32, Negate)              , NULL  ),          // 0x8c
    M3OP_F( "f32.ceil",         0,  f_32,   d_unaryOpList(f32, Ceil)                , NULL  ),          // 0x8d
    M3OP_F( "f32.floor",        0,  f_32,   d_unaryOpList(f32, Floor)               , NULL  ),          // 0x8e
    M3OP_F( "f32.trunc",        0,  f_32,   d_unaryOpList(f32, Trunc)               , NULL  ),          // 0x8f
    M3OP_F( "f32.nearest",      0,  f_32,   d_unaryOpList(f32, Nearest)             , NULL  ),          // 0x90
    M3OP_F( "f32.sqrt",         0,  f_32,   d_unaryOpList(f32, Sqrt)                , NULL  ),          // 0x91

    M3OP_F( "f32.add",          -1, f_32,   d_commutativeBinOpList (f32, Add)       , NULL  ),          // 0x92
    M3OP_F( "f32.sub",          -1, f_32,   d_binOpList (f32, Subtract)             , NULL  ),          // 0x93
    M3OP_F( "f32.mul",          -1, f_32,   d_commutativeBinOpList (f32, Multiply)  , NULL  ),          // 0x94
    M3OP_F( "f32.div",          -1, f_32,   d_binOpList (f32, Divide)               , NULL  ),          // 0x95
    M3OP_F( "f32.min",          -1, f_32,   d_commutativeBinOpList (f32, Min)       , NULL  ),          // 0x96
    M3OP_F( "f32.max",          -1, f_32,   d_commutativeBinOpList (f32, Max)       , NULL  ),          // 0x97
    M3OP_F( "f32.copysign",     -1, f_32,   d_binOpList (f32, CopySign)             , NULL  ),          // 0x98

    M3OP_F( "f64.abs",          0,  f_64,   d_unaryOpList(f64, Abs)                 , NULL  ),          // 0x99
    M3OP_F( "f64.neg",          0,  f_64,   d_unaryOpList(f64, Negate)              , NULL  ),          // 0x9a
    M3OP_F( "f64.ceil",         0,  f_64,   d_unaryOpList(f64, Ceil)                , NULL  ),          // 0x9b
    M3OP_F( "f64.floor",        0,  f_64,   d_unaryOpList(f64, Floor)               , NULL  ),          // 0x9c
    M3OP_F( "f64.trunc",        0,  f_64,   d_unaryOpList(f64, Trunc)               , NULL  ),          // 0x9d
    M3OP_F( "f64.nearest",      0,  f_64,   d_unaryOpList(f64, Nearest)             , NULL  ),          // 0x9e
    M3OP_F( "f64.sqrt",         0,  f_64,   d_unaryOpList(f64, Sqrt)                , NULL  ),          // 0x9f

    M3OP_F( "f64.add",          -1, f_64,   d_commutativeBinOpList (f64, Add)       , NULL  ),          // 0xa0
    M3OP_F( "f64.sub",          -1, f_64,   d_binOpList (f64, Subtract)             , NULL  ),          // 0xa1
    M3OP_F( "f64.mul",          -1, f_64,   d_commutativeBinOpList (f64, Multiply)  , NULL  ),          // 0xa2
    M3OP_F( "f64.div",          -1, f_64,   d_binOpList (f64, Divide)               , NULL  ),          // 0xa3
    M3OP_F( "f64.min",          -1, f_64,   d_commutativeBinOpList (f64, Min)       , NULL  ),          // 0xa4
    M3OP_F( "f64.max",          -1, f_64,   d_commutativeBinOpList (f64, Max)       , NULL  ),          // 0xa5
    M3OP_F( "f64.copysign",     -1, f_64,   d_binOpList (f64, CopySign)             , NULL  ),          // 0xa6

    M3OP( "i32.wrap/i64",       0,  i_32,   d_unaryOpList (i32, Wrap_i64),          NULL    ),          // 0xa7
    M3OP_F( "i32.trunc_s/f32",  0,  i_32,   d_convertOpList (i32_Trunc_f32),        Compile_Convert ),  // 0xa8
    M3OP_F( "i32.trunc_u/f32",  0,  i_32,   d_convertOpList (u32_Trunc_f32),        Compile_Convert ),  // 0xa9
    M3OP_F( "i32.trunc_s/f64",  0,  i_32,   d_convertOpList (i32_Trunc_f64),        Compile_Convert ),  // 0xaa
    M3OP_F( "i32.trunc_u/f64",  0,  i_32,   d_convertOpList (u32_Trunc_f64),        Compile_Convert ),  // 0xab

    M3OP( "i64.extend_s/i32",   0,  i_64,   d_unaryOpList (i64, Extend_i32),        NULL    ),          // 0xac
    M3OP( "i64.extend_u/i32",   0,  i_64,   d_unaryOpList (i64, Extend_u32),        NULL    ),          // 0xad

    M3OP_F( "i64.trunc_s/f32",  0,  i_64,   d_convertOpList (i64_Trunc_f32),        Compile_Convert ),  // 0xae
    M3OP_F( "i64.trunc_u/f32",  0,  i_64,   d_convertOpList (u64_Trunc_f32),        Compile_Convert ),  // 0xaf
    M3OP_F( "i64.trunc_s/f64",  0,  i_64,   d_convertOpList (i64_Trunc_f64),        Compile_Convert ),  // 0xb0
    M3OP_F( "i64.trunc_u/f64",  0,  i_64,   d_convertOpList (u64_Trunc_f64),        Compile_Convert ),  // 0xb1

    M3OP_F( "f32.convert_s/i32",0,  f_32,   d_convertOpList (f32_Convert_i32),      Compile_Convert ),  // 0xb2
    M3OP_F( "f32.convert_u/i32",0,  f_32,   d_convertOpList (f32_Convert_u32),      Compile_Convert ),  // 0xb3
    M3OP_F( "f32.convert_s/i64",0,  f_32,   d_convertOpList (f32_Convert_i64),      Compile_Convert ),  // 0xb4
    M3OP_F( "f32.convert_u/i64",0,  f_32,   d_convertOpList (f32_Convert_u64),      Compile_Convert ),  // 0xb5

    M3OP_F( "f32.demote/f64",   0,  f_32,   d_unaryOpList (f32, Demote_f64),        NULL    ),          // 0xb6

    M3OP_F( "f64.convert_s/i32",0,  f_64,   d_convertOpList (f64_Convert_i32),      Compile_Convert ),  // 0xb7
    M3OP_F( "f64.convert_u/i32",0,  f_64,   d_convertOpList (f64_Convert_u32),      Compile_Convert ),  // 0xb8
    M3OP_F( "f64.convert_s/i64",0,  f_64,   d_convertOpList (f64_Convert_i64),      Compile_Convert ),  // 0xb9
    M3OP_F( "f64.convert_u/i64",0,  f_64,   d_convertOpList (f64_Convert_u64),      Compile_Convert ),  // 0xba

    M3OP_F( "f64.promote/f32",  0,  f_64,   d_unaryOpList (f64, Promote_f32),       NULL    ),          // 0xbb

    M3OP_F( "i32.reinterpret/f32",0,i_32,   d_convertOpList (i32_Reinterpret_f32),  Compile_Convert ),  // 0xbc
    M3OP_F( "i64.reinterpret/f64",0,i_64,   d_convertOpList (i64_Reinterpret_f64),  Compile_Convert ),  // 0xbd
    M3OP_F( "f32.reinterpret/i32",0,f_32,   d_convertOpList (f32_Reinterpret_i32),  Compile_Convert ),  // 0xbe
    M3OP_F( "f64.reinterpret/i64",0,f_64,   d_convertOpList (f64_Reinterpret_i64),  Compile_Convert ),  // 0xbf

    M3OP( "i32.extend8_s",       0,  i_32,   d_unaryOpList (i32, Extend8_s),        NULL    ),          // 0xc0
    M3OP( "i32.extend16_s",      0,  i_32,   d_unaryOpList (i32, Extend16_s),       NULL    ),          // 0xc1
    M3OP( "i64.extend8_s",       0,  i_64,   d_unaryOpList (i64, Extend8_s),        NULL    ),          // 0xc2
    M3OP( "i64.extend16_s",      0,  i_64,   d_unaryOpList (i64, Extend16_s),       NULL    ),          // 0xc3
    M3OP( "i64.extend32_s",      0,  i_64,   d_unaryOpList (i64, Extend32_s),       NULL    ),          // 0xc4

# ifdef DEBUG // for codepage logging. the order doesn't matter:
#   define d_m3DebugOp(OP, IDX) M3OP (#OP, IDX, 0, none, { op_##OP })

# if d_m3HasFloat
#   define d_m3DebugTypedOp(OP, IDX) M3OP (#OP, IDX, 0, none, { op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64, })
# else
#   define d_m3DebugTypedOp(OP, IDX) M3OP (#OP, IDX, 0, none, { op_##OP##_i32, op_##OP##_i64 })
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (Entry),            d_m3DebugOp (End),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),

    d_m3DebugOp (CopySlot_32),      d_m3DebugOp (PreserveCopySlot_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (CopySlot_64),      d_m3DebugOp (PreserveCopySlot_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# if d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),

    d_m3DebugTypedOp (SetGlobal),   d_m3DebugOp (SetGlobal_s32),    d_m3DebugOp (SetGlobal_s64),

    d_m3DebugTypedOp (SetRegister), d_m3DebugTypedOp (SetSlot),     d_m3DebugTypedOp (PreserveSetSlot),
# endif

# if d_m3CascadedOpcodes
    [c_waOp_extended] = M3OP( "0xFC", 0, c_m3Type_unknown,   d_emptyOpList,  Compile_ExtendedOpcode ),
# endif

# if DEBUG
    M3OP( "termination", 0, c_m3Type_unknown ), // for find_operation_info
# endif

#if WASM_ENABLE_OP_TRACE && WASM_DEBUG_DumpStack_InOps
    d_m3DebugOp (DumpStack)
#endif
};

const M3OpInfo c_operationsFC [] =
{
    M3OP_F( "i32.trunc_s:sat/f32",0,  i_32,   d_convertOpList (i32_TruncSat_f32),        Compile_Convert ),  // 0x00
    M3OP_F( "i32.trunc_u:sat/f32",0,  i_32,   d_convertOpList (u32_TruncSat_f32),        Compile_Convert ),  // 0x01
    M3OP_F( "i32.trunc_s:sat/f64",0,  i_32,   d_convertOpList (i32_TruncSat_f64),        Compile_Convert ),  // 0x02
    M3OP_F( "i32.trunc_u:sat/f64",0,  i_32,   d_convertOpList (u32_TruncSat_f64),        Compile_Convert ),  // 0x03
    M3OP_F( "i64.trunc_s:sat/f32",0,  i_64,   d_convertOpList (i64_TruncSat_f32),        Compile_Convert ),  // 0x04
    M3OP_F( "i64.trunc_u:sat/f32",0,  i_64,   d_convertOpList (u64_TruncSat_f32),        Compile_Convert ),  // 0x05
    M3OP_F( "i64.trunc_s:sat/f64",0,  i_64,   d_convertOpList (i64_TruncSat_f64),        Compile_Convert ),  // 0x06
    M3OP_F( "i64.trunc_u:sat/f64",0,  i_64,   d_convertOpList (u64_TruncSat_f64),        Compile_Convert ),  // 0x07

    M3OP_RESERVED, M3OP_RESERVED,

    M3OP( "memory.copy",            0,  none,   d_emptyOpList,                           Compile_Memory_CopyFill ), // 0x0a
    M3OP( "memory.fill",            0,  none,   d_emptyOpList,                           Compile_Memory_CopyFill ), // 0x0b


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown ) // for find_operation_info    
# endif
};
