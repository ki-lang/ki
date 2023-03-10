
enum TYPES {
    type_void,
    type_ptr,
    type_struct,
    type_int,
    type_float,
    type_func_ref,
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
