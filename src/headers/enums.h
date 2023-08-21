
enum BUILD_TYPES {
    build_t_exe,
    build_t_shared_lib,
};

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
    idf_err_code,
    idf_macro,
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
    v_this_or_that
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
    os_other,
    os_linux,
    os_macos,
    os_win,
};
enum TARGET_ARCH {
    arch_other,
    arch_x64,
    arch_arm64,
};

enum ALIAS {
    alias_id,
    alias_type,
};

enum MACRO_PART_TYPE {
    macro_part_type,
    macro_part_value,
};

enum LSP_DATA_TYPE {
    lspt_completion,
    lspt_definition,
    lspt_sig_help,
    lspt_diagnostic,
};

enum LSP_COMPLETION_TYPE {
	lsp_compl_text = 1,
	lsp_compl_method = 2,
	lsp_compl_function = 3,
	lsp_compl_constructor = 4,
	lsp_compl_field = 5,
	lsp_compl_variable = 6,
	lsp_compl_class = 7,
	lsp_compl_interface = 8,
	lsp_compl_module = 9,
	lsp_compl_property = 10,
	lsp_compl_unit = 11,
	lsp_compl_value = 12,
	lsp_compl_enum = 13,
	lsp_compl_keyword = 14,
	lsp_compl_snippet = 15,
	lsp_compl_color = 16,
	lsp_compl_file = 17,
	lsp_compl_reference = 18,
	lsp_compl_folder = 19,
	lsp_compl_enumMember = 20,
	lsp_compl_constant = 21,
	lsp_compl_struct = 22,
	lsp_compl_event = 23,
	lsp_compl_operator = 24,
	lsp_compl_typeParameter = 25,
};