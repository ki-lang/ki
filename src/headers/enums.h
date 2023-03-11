
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
};

enum VALUETYPE {
    v_string,
    v_var,
    v_vint,
    v_ptrv,
    v_op,
    v_compare,
    v_fcall,
    v_fptr,
    v_cast,
    v_class_pa,
};

enum OPTYPE {
    op_add,
    op_sub,
    op_eq,
    op_ne,
    op_lt,
    op_gt,
    op_lte,
    op_gte,
};