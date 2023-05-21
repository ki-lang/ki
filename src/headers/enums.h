
enum TYPES {
    type_void,
    type_ptr,
    type_struct,
    type_int,
    type_float,
    type_func_ptr,
    type_null,
    type_arr,
};

enum IDFOR {
    idf_func,
    idf_class,
    idf_decl,
    idf_nsc,
    idf_trait,
    idf_enum,
    idf_fc,
    idf_global,
    idf_type,
    idf_decl_type_overwrite,
};

enum SCOPETYPE {
    sct_default,
    sct_func,
    sct_class,
    sct_loop,
    sct_fc,
    sct_vscope,
};

enum CLASSTYPE {
    ct_struct,
    ct_int,
    ct_float,
    ct_ptr,
};
enum ACCESSTYPE {
    act_public,
    act_private,
    act_readonly,
};

enum TOKENTYPE {
    tkn_declare,
    tkn_assign,
    tkn_statement,
    tkn_return,
    tkn_while,
    tkn_if,
    tkn_break,
    tkn_continue,
    tkn_throw,
    tkn_panic,
    tkn_exit,
    tkn_each,
    tkn_ref,
    tkn_ir_val,
    tkn_ir_assign_val,
    tkn_exec,
    tkn_vscope_return,
};

enum VALUETYPE {
    v_string,
    v_fstring,
    v_vint,
    v_float,
    v_ptrv,
    v_op, // 5
    v_compare,
    v_fcall,
    v_fptr,
    v_cast,
    v_class_pa, // 10
    v_class_init,
    v_null,
    v_or_break,
    v_or_value,
    v_decl, // 15
    v_and_or,
    v_getptr,
    v_ir_value,
    v_upref_value,
    v_ir_val,
    v_ir_from,
    v_ir_assign_val,
    v_ir_raw_val,
    v_ir_load,
    v_global,
    v_value_and_exec,
    v_value_then_ir_value,
    v_incr_decr,
    v_atomicop,
    v_stack_alloc,
    v_scope,
    v_array_item,
    v_isset,
    v_ref,
    v_swap,
    v_ptrval,
};

enum OPTYPE {
    op_add,
    op_sub,
    op_mul,
    op_div,
    op_mod,
    op_bit_and,
    op_bit_or,
    op_bit_xor,
    op_shl,
    op_shr,
    //
    op_eq,
    op_ne,
    op_lt,
    op_gt,
    op_lte,
    op_gte,
    //
    op_and,
    op_or,
};

enum READTYPECTX {
    rtc_default,
    rtc_func_arg,
    rtc_func_rett,
    rtc_ptrv,
    rtc_decl,
    rtc_prop_type,
    rtc_sub_type,
};

enum LINKTYPE {
    link_default = 1,
    link_dynamic = 2,
    link_static = 4,
};

enum TARGET_OS {
    os_linux,
    os_macos,
    os_win,
};
enum TARGET_ARCH {
    arch_x64,
    arch_arm64,
};

enum ALIAS {
    alias_id,
    alias_type,
};