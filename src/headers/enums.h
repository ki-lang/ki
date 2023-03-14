
enum TYPES {
    type_void,
    type_ptr,
    type_struct,
    type_int,
    type_float,
    type_func_ptr,
    type_null,
};

enum IDFOR {
    idf_func,
    idf_class,
    idf_var,
    idf_nsc,
    idf_trait,
    idf_enum,
    idf_fc,
};

enum SCOPETYPE {
    sct_default,
    sct_func,
    sct_class,
    sct_loop,
    sct_fc,
};

enum CLASSTYPE {
    ct_struct,
    ct_int,
    ct_float,
    ct_ptr,
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
    tkn_upref_slot,
    tkn_ref,
    tkn_tmp_var,
};

enum VALUETYPE {
    v_string,
    v_var,
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
    v_tmp_var,
    v_null,
    v_or_break,
    v_or_value, // 15
    v_decl,
    v_and_or,
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