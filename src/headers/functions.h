
// General
void die(char *msg);
void parse_argv(char **argv, int argc, Array *has_value, Array *args, Map *options);
char *rand_string(char *str, int size);
int atoi(const char *str);
int hex2int(char *hex);
void sleep_ns(unsigned int ns);

// Syntax
bool is_alpha_char(char c);
bool is_valid_varname_char(char c);
bool is_number(char c);
bool is_hex_char(char c);
bool is_whitespace(char c);
bool is_newline(char c);
bool is_valid_varname(char *name);
bool is_valid_number(char *str);
bool is_valid_hex_number(char *str);
bool is_valid_macro_number(char *str);
bool ends_with(const char *str, const char *suffix);
bool starts_with(const char *a, const char *b);

// Alloc
Allocator *alc_make();
void alc_wipe(Allocator *alc);
AllocatorBlock *alc_block_make(AllocatorBlock *prev, AllocatorBlock *next, size_t size);
void *al(Allocator *alc, size_t size);
AllocatorBlock *al_private(Allocator *alc, size_t size);
void free_block(AllocatorBlock *block);
char *dups(Allocator *alc, char *str);

// Build
void cmd_build(int argc, char **argv);
Class *ki_get_class(Build *b, char *ns, char *name);
Func *ki_get_func(Build *b, char *ns, char *name);

// Chain
Chain *chain_make(Allocator *alc);
Fc *chain_get(Chain *chain);
void chain_add(Chain *chain, Fc *item);

// Loop
void *io_loop(void *build);
void compile_loop(Build *b, int max_stage);

// Pkc
Pkc *pkc_init(Allocator *alc, Build *b, char *name, char *dir);
Nsc *pkc_get_nsc(Pkc *pkc, char *name);
Nsc *pkc_load_nsc(Pkc *pkc, char *name, Fc *parsing_fc);
void pkc_cfg_save(Config *cfg);
Pkc *pkc_get_sub_package(Pkc *pkc, char *name);

// Nsc
Nsc *nsc_init(Allocator *alc, Build *b, Pkc *pkc, char *name);
char *nsc_gname(Nsc *nsc, char *name);
char *nsc_dname(Nsc *nsc, char *name);

// Fc
Fc *fc_init(Build *b, char *path_ki, Nsc *nsc, bool generated);
void fc_error(Fc *fc);
void fc_update_cahce(Fc *fc);

//
void stage_1(Fc *);
void stage_2(Fc *);
void stage_2_class(Fc *fc, Class *class);
void stage_2_class_defaults(Fc *fc, Class *class);
void stage_2_func(Fc *fc, Func *func);
void stage_3(Fc *);
void stage_4(Fc *);
void stage_5(Fc *);
void stage_6(Fc *);
void stage_7(Fc *);
void stage_8(Build *b);

// Read
Chunk *chunk_init(Allocator *alc, Fc *fc);
Chunk *chunk_clone(Allocator *alc, Chunk *chunk);
void chunk_move(Chunk *chunk, int pos);
void tok(Fc *fc, char *token, bool sameline, bool allow_space);
void rtok(Fc *fc);
void tok_expect(Fc *fc, char *expect, bool sameline, bool allow_space);
char get_char(Fc *fc, int index);
void read_hex(Fc *fc, char *token);
Str *read_string(Fc *fc);

// Skips
void skip_body(Fc *fc, char until_ch);
void skip_string(Fc *fc, char end_char);
void skip_until_char(Fc *fc, char *find);
void skip_whitespace(Fc *fc);
void skip_macro_if(Fc *fc);
void skip_traits(Fc *fc);
void skip_value(Fc *fc);
void skip_type(Fc *fc);

// Macro
MacroScope *init_macro_scope(Allocator *alc);
void read_macro(Fc *fc, Allocator *alc, Scope *scope);
bool macro_resolve_if_value(Fc *fc, Scope *scope, MacroScope *mc);
char *macro_get_var(MacroScope *mc, char *key);

// Id
Id *id_init(Allocator *alc);
Idf *idf_init(Allocator *alc, int type);
Id *read_id(Fc *fc, bool sameline, bool allow_space, bool crash);
Idf *idf_by_id(Fc *fc, Scope *scope, Id *id, bool fail);
Idf *ki_lib_get(Build *b, char *ns, char *name);

// Scope
Scope *scope_init(Allocator *alc, int type, Scope *parent, bool has_ast);
void name_taken_check(Fc *fc, Scope *scope, char *name);
Scope *scope_find(Scope *scope, int type);

// Func
Func *func_init(Allocator *alc);
void fcall_type_check(Fc *fc, Value *on, Array *values);
void func_make_arg_decls(Func *func);

// Class
Class *class_init(Allocator *alc);
ClassProp *class_prop_init(Allocator *alc, Class *class, Type *type);
bool class_check_size(Class *class);
Func *class_define_func(Fc *fc, Class *class, bool is_static, char *name, Array *args, Type *rett);
void class_ref_change(Allocator *alc, Scope *scope, Value *on, int amount);
void class_free_value(Allocator *alc, Scope *scope, Value *value);
void class_generate_generic_hash(Class *class, Array *types, char *buf);
Class *class_get_generic_class(Class *class, Array *types);
Array *read_generic_types(Fc *fc, Scope *scope, Class *class);

// Type
Type *type_init(Allocator *alc);
int type_get_size(Build *b, Type *type);
bool type_is_void(Type *type);
bool type_is_ptr(Type *type, Build *b);
bool type_is_bool(Type *type, Build *b);
Type *type_gen_class(Allocator *alc, Class *class);
Type *type_gen_fptr(Allocator *alc, Func *func);
Type *type_gen_int(Build *b, Allocator *alc, int bytes, bool is_signed);
Type *type_gen_void(Allocator *alc);
Type *type_gen(Build *b, Allocator *alc, char *name);
Type *read_type(Fc *fc, Allocator *alc, Scope *scope, bool sameline, bool allow_space);
bool type_compat(Type *t1, Type *t2, char **reason);
char *type_to_str(Type *t, char *res);
void type_check(Fc *fc, Type *t1, Type *t2);
Type *type_clone(Allocator *alc, Type *type);

// Var
Decl *decl_init(Allocator *alc, Scope *scope, char *name, Type *type, Value *val, bool is_mut, bool is_arg, bool keep);
Var *var_init(Allocator *alc, Decl *decl, Type *type);
Arg *arg_init(Allocator *alc, char *name, Type *type, bool is_mut);

// UsageLine
UsageLine *usage_line_init(Allocator *alc, Scope *scope, Decl *decl);
UsageLine *usage_line_get(Scope *scope, Decl *decl);
bool is_moved_once(UsageLine *ul);
void usage_read_value(Allocator *alc, Scope *scope, Value *val);
Value *usage_move_value(Allocator *alc, Chunk *chunk, Scope *scope, Value *val);
Scope *usage_scope_init(Allocator *alc, Scope *parent, int type);
void usage_merge_ancestors(Allocator *alc, Scope *left, Array *ancestors);
void usage_collect_used_decls(Allocator *alc, Scope *left, Scope *right, Array **list);
void end_usage_line(Allocator *alc, UsageLine *ul);
void deref_scope(Allocator *alc, Scope *scope, Scope *until);
void deref_expired_decls(Allocator *alc, Scope *scope);
void usage_clear_ancestors(Scope *scope);
Scope *usage_create_deref_scope(Allocator *alc, Scope *scope);

// Value
Value *value_init(Allocator *alc, int type, void *item, Type *rett);
Value *read_value(Fc *fc, Allocator *alc, Scope *scope, bool sameline, int prio, bool assignable);
Value *value_op(Fc *fc, Allocator *alc, Scope *scope, Value *left, Value *right, int op);
Value *try_convert(Fc *fc, Allocator *alc, Value *val, Type *to_type);
bool value_is_assignable(Value *v);

Value *vgen_vint(Allocator *alc, long int value, Type *type, bool force_type);
Value *vgen_vfloat(Allocator *alc, Build *b, float value, bool force_type);
Value *vgen_ptrv(Allocator *alc, Value *on, Type *as);
Value *vgen_op(Allocator *alc, Build *b, Value *left, Value *right, int op, bool is_ptr);
Value *vgen_compare(Allocator *alc, Build *b, Value *left, Value *right, int op);
Value *vgen_fcall(Allocator *alc, Value *on, Array *values, Type *rett, FCallOr * or);
Value *vgen_fptr(Allocator *alc, Func *func, Value *first_arg);
Value *vgen_class_pa(Allocator *alc, Value *on, ClassProp *prop);
Value *vgen_class_init(Allocator *alc, Class *class, Map *values);
Value *vgen_cast(Allocator *alc, Value *val, Type *to_type);
Value *vgen_null(Allocator *alc, Build *b);
Value *vgen_or_break(Allocator *alc, Value *value, Scope *or_scope, Scope *else_scope, Scope *deref_scope);
Value *vgen_or_value(Allocator *alc, Value *left, Value *right, Scope *value_scope, Scope *else_scope, Scope *deref_scope);
Value *vgen_and_or(Allocator *alc, Build *b, Value *left, Value *right, int op);
Value *vgen_ir_val(Allocator *alc, Value *value, Type *rett);
Value *vgen_ir_assign_val(Allocator *alc, Value *value, Type *rett);
Value *vgen_value_and_exec(Allocator *alc, Value *value, Scope *exec_scope, bool before_value, bool enable_exec);
Value *vgen_value_then_ir_value(Allocator *alc, Value *value);
Value *vgen_incr_decr(Allocator *alc, Value *on, bool is_incr);

// Ast
void read_ast(Fc *fc, Scope *scope, bool single_line);

// Token
Token *token_init(Allocator *alc, int type, void *item);
TIf *tgen_tif(Allocator *alc, Value *cond, Scope *scope, Scope *else_scope, Scope *deref_scope);
Token *tgen_declare(Allocator *alc, Decl *decl, Value *val);
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Scope *fscope, Value *retv);
Token *tgen_while(Allocator *alc, Value *cond, Scope *scope);
Token *tgen_exec(Allocator *alc, Scope *scope, bool enable);
Token *tgen_each(Allocator *alc, Value *value, Scope *scope, char *key_name, char *value_name);
