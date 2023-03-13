
// General
void die(char *msg);
void parse_argv(char **argv, int argc, Array *has_value, Array *args, Map *options);
char *rand_string(char *str, int size);
int atoi(const char *str);
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
Fc *fc_init(Build *b, char *path_ki, Nsc *nsc);
void fc_error(Fc *fc);

//
void stage_1(Fc *);
void stage_2(Fc *);
void stage_3(Fc *);
void stage_4(Fc *);
void stage_5(Fc *);
void stage_6(Fc *);
void stage_7(Fc *);
void stage_8(Build *b);

// Read
Chunk *chunk_init(Allocator *alc);
Chunk *chunk_clone(Allocator *alc, Chunk *chunk);
void chunk_move(Chunk *chunk, int pos);
void tok(Fc *fc, char *token, bool sameline, bool allow_space);
void rtok(Fc *fc);
void tok_expect(Fc *fc, char *expect, bool sameline, bool allow_space);
char get_char(Fc *fc, int index);
Str *read_string(Fc *fc);

// Skips
void skip_body(Fc *fc, char until_ch);
void skip_string(Fc *fc, char end_char);
void skip_until_char(Fc *fc, char *find);
void skip_whitespace(Fc *fc);
void skip_macro_if(Fc *fc);
void skip_traits(Fc *fc);
void skip_value(Fc *fc);

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

// Var
Decl *decl_init(Allocator *alc, Scope *scope, char *name, Type *type, Value *val, bool is_mut, bool is_arg, bool is_global);
Var *var_init(Allocator *alc, Decl *decl, Type *type);
Arg *arg_init(Allocator *alc, char *name, Type *type, bool is_mut);
UprefSlot *upref_slot_init(Allocator *alc, Decl *decl);

// Value
Value *value_init(Allocator *alc, int type, void *item, Type *rett);
Value *read_value(Fc *fc, Allocator *alc, Scope *scope, bool sameline, int prio);
Value *value_op(Fc *fc, Allocator *alc, Scope *scope, Value *left, Value *right, int op);
Value *try_convert(Fc *fc, Allocator *alc, Value *val, Type *to_type);
bool value_assignable(Value *val);
void upref_value_check(Allocator *alc, Scope *scope, Value *val);

Value *vgen_vint(Allocator *alc, long int value, Type *type, bool force_type);
Value *vgen_vfloat(Allocator *alc, Build *b, float value, bool force_type);
Value *vgen_ptrv(Allocator *alc, Value *on, Type *as);
Value *vgen_op(Allocator *alc, Build *b, Value *left, Value *right, int op, bool is_ptr);
Value *vgen_compare(Allocator *alc, Build *b, Value *left, Value *right, int op);
Value *vgen_fcall(Allocator *alc, Value *on, Array *args, Type *rett, Scope *scope);
Value *vgen_fptr(Allocator *alc, Func *func, Value *first_arg);
Value *vgen_class_pa(Allocator *alc, Value *on, ClassProp *prop);
Value *vgen_class_init(Allocator *alc, Class *class, Map *values);
Value *vgen_cast(Allocator *alc, Value *val, Type *to_type);

// Ast
void read_ast(Fc *fc, Scope *scope, bool single_line);

// Token
Token *token_init(Allocator *alc, int type, void *item);
TIf *tgen_tif(Allocator *alc, Value *cond, Scope *scope, TIf *else_if);
Token *tgen_declare(Allocator *alc, Decl *decl, Value *val);
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Scope *fscope, Value *retv);
Token *tgen_while(Allocator *alc, Value *cond, Scope *scope);
