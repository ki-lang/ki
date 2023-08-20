
// General
void die(char *msg);
void parse_argv(char **argv, int argc, Array *has_value, Array *args, Map *options);
char *rand_string(char *str, int size);
int atoi(const char *str);
int hex2int(char *hex);
void sleep_ns(unsigned int ns);
void sleep_ms(unsigned int ms);
void simple_hash(char *content, char *buf);
Array *explode(Allocator *alc, char *part, char *content);
int system_silent(char *cmd);
char *str_replace_simple(char *s, const char *s1, const char *s2);
char *str_replace(Allocator *alc, char *orig, char *rep, char *with);
void free_delayed(void *item);
void rtrim(char *str, char ch);
void run_async(void *func, void *arg, bool wait);

// Syntax
bool is_alpha_char(char c);
bool is_valid_varname_char(char c);
bool is_number(char c);
bool is_hex_char(char c);
bool is_whitespace(char c);
bool is_newline(char c);
bool is_valid_varname(char *name);
bool is_valid_varname_all(char *name);
bool is_valid_number(char *str);
bool is_valid_hex_number(char *str);
bool is_valid_macro_number(char *str);
bool ends_with(const char *str, const char *suffix);
bool starts_with(const char *a, const char *b);

// Alloc
Allocator *alc_make();
void alc_wipe(Allocator *alc);
void alc_delete(Allocator *alc);
AllocatorBlock *alc_block_make(AllocatorBlock *prev, AllocatorBlock *next, size_t size);
void *al(Allocator *alc, size_t size);
AllocatorBlock *al_private(Allocator *alc, size_t size);
void free_block(AllocatorBlock *block);
char *dups(Allocator *alc, char *str);

// Build
void cmd_build(int argc, char **argv, LspData *lsp_data);
void build_end(Build *b, int exit_code);
void build_error(Build *b, char *msg);
Class *ki_get_class(Build *b, char *ns, char *name);
Func *ki_get_func(Build *b, char *ns, char *name);

// Loader
char *loader_find_config_dir(Build *b, char *dir);
Pkc *loader_get_pkc_for_dir(Build *b, char *dir);
// Pkc *loader_find_pkc_for_file(Build *b, char *path);
// Nsc *loader_get_nsc_for_file(Pkc *pkc, char *path);
Nsc *loader_get_nsc_for_dir(Build *b, char *dir);
Nsc *loader_load_nsc(Pkc *pkc, char *name);

// Pkg
void cmd_pkg(int argc, char *argv[]);
void pkg_add(PkgCmd *pc, char *name, char *version, char *alias);
void pkg_remove(PkgCmd *pc, char *name);
bool pkg_install_package(PkgCmd *pc, char *dir, char *name, char *version, char *clone_url, char *hash);
void pkg_install(PkgCmd *pc);
void pkg_get_dir(char *packages_dir, char *name, char *buf);

// Make
void cmd_make(int argc, char *argv[]);

// LSP
void cmd_lsp(int argc, char *argv[]);
cJSON *lsp_init(Allocator *alc, cJSON *params);
bool lsp_check(Fc *fc);
void lsp_log(char *msg);
int lsp_get_pos_index(char *text, int line, int col);
void lsp_run_build(LspData *ld);
void *lsp_run_build_entry(void *ld_);
void *lsp_run_build_entry_2(void *ld_);
cJSON *lsp_open(Allocator *alc, cJSON *params);
cJSON *lsp_close(Allocator *alc, cJSON *params);
cJSON *lsp_change(Allocator *alc, cJSON *params);
cJSON *lsp_save(Allocator *alc, cJSON *params);
cJSON *lsp_definition(Allocator *alc, cJSON *params, int id);
cJSON *lsp_completion(Allocator *alc, cJSON *params, int id);
cJSON *lsp_help(Allocator *alc, cJSON *params, int id);
void lsp_help_respond(Build *b, LspData *ld, char *full, Array *args, int arg_index);
void lsp_help_check_args(Allocator *alc, Fc *fc, Array *args, bool skip_first, Type *rett, int arg_index);
void lsp_completion_respond(Allocator *alc, LspData* ld, Array *items);
void lsp_definition_respond(Allocator *alc, LspData *ld, char *path, int line, int col);
void lsp_diagnostic_respond(Allocator *alc, LspData *ld, Array *errors_);
void lsp_respond(cJSON *resp);
void lsp_exit_thread();

// LSP structs
LspData* lsp_data_init();
void lsp_data_free(LspData *ld);
LspCompletion *lsp_completion_init(Allocator *alc, int type, char *label);

// LSP labels
char *lsp_func_label(Allocator *alc, Func *func, char *name, bool skip_first_arg);
char *lsp_func_insert(Allocator *alc, Func *func, char *name, bool skip_first_arg);
char *lsp_func_help(Allocator *alc, Array *args, bool skip_first_arg, Type *rett);

// Config
Config *cfg_load(Allocator *alc, Str *buf, char *dir);
bool cfg_has_package(Config *cfg, char *name);
void cfg_save(Config *cfg);

// Version
PkgVersion *extract_version(char *content);
bool is_higher_version_than(PkgVersion *new, PkgVersion *than);
bool is_same_version(PkgVersion *a, PkgVersion *b);
void version_to_str(PkgVersion *v, char *buf);

// Github
bool pkg_is_github_url(char *name);
char *github_find_version_hash(GithubPkg *ghub, char *version);
GithubPkg *github_parse_url(Allocator *alc, char *name);
char *github_full_commit_hash(GithubPkg *ghub, char *shash);

// Chain
Chain *chain_make(Allocator *alc);
Fc *chain_get(Chain *chain);
void chain_add(Chain *chain, Fc *item);

// Loop
void *io_loop(void *build);
void compile_loop(Build *b, int max_stage);

// Pkc
Pkc *pkc_init(Allocator *alc, Build *b, char *name, char *dir, Config *cfg);
Nsc *pkc_get_nsc(Pkc *pkc, char *name);
Pkc *pkc_get_sub_package(Pkc *pkc, char *name);

// Nsc
Nsc *nsc_init(Allocator *alc, Build *b, Pkc *pkc, char *name);
char *nsc_gname(Fc *fc, char *name);
char *nsc_dname(Fc *fc, char *name);

// Fc
Fc *fc_init(Build *b, char *path_ki, Nsc *nsc, bool duplicate);
void fc_set_cache_paths(Fc *fc);
void fc_error(Fc *fc);
void fc_update_cache(Fc *fc);

//
void stage_1(Fc *);
void stage_2(Fc *);
void stage_2_class(Fc *fc, Class *class);
void stage_2_class_defaults(Fc *fc, Class *class);
void stage_2_class_type_checks(Fc *fc, Class *class);
void stage_2_class_props(Fc *fc, Class *class, bool is_trait, bool is_extend);
void stage_2_func(Fc *fc, Func *func);
void stage_2_1(Fc *fc);
void stage_3(Fc *);
void stage_3_circular(Build *b, Class *class);
void stage_4(Fc *);
void stage_5(Fc *);
void stage_6(Fc *);
void stage_7(Fc *);
void stage_8(Build *b);

// Read
Chunk *chunk_init(Allocator *alc, Fc *fc);
Chunk *chunk_clone(Allocator *alc, Chunk *chunk);
void chunk_move(Chunk *chunk, int pos);
void chunk_update_col(Chunk *chunk);
void tok(Fc *fc, char *token, bool sameline, bool allow_space);
void rtok(Fc *fc);
void tok_expect(Fc *fc, char *expect, bool sameline, bool allow_space);
char get_char(Fc *fc, int index);
void read_hex(Fc *fc, char *token);
Str *read_string(Fc *fc);
Array *read_string_chunks(Allocator *alc, Fc *fc);
char *read_part(Allocator *alc, Fc *fc, int i, int len);

// Skips
void skip_body(Fc *fc, char until_ch);
void skip_string(Fc *fc, char end_char);
void skip_until_char(Fc *fc, char *find);
void skip_whitespace(Fc *fc);
void skip_macro_if(Fc *fc);
void skip_traits(Fc *fc);
void skip_value(Fc *fc);
void skip_type(Fc *fc);
void skip_macro_input(Fc *fc, char *end);

// Macro
MacroScope *init_macro_scope(Allocator *alc);
void read_macro(Fc *fc, Allocator *alc, Scope *scope);
bool macro_resolve_if_value(Fc *fc, Scope *scope, MacroScope *mc);
char *macro_get_var(MacroScope *mc, char *key);
Str *macro_replace_str_vars(Allocator *alc, Fc *fc, Str *str);

// Id
Id *id_init(Allocator *alc);
Idf *idf_init(Allocator *alc, int type);
Idf *idf_init_item(Allocator *alc, int type, void *item);
Id *read_id(Fc *fc, bool sameline, bool allow_space, bool crash);
Idf *read_idf(Fc *fc, Scope *scope, bool sameline, bool allow_space);
Idf *idf_by_id(Fc *fc, Scope *scope, Id *id, bool fail);
Idf *ki_lib_get(Build *b, char *ns, char *name);
Idf *idf_get_from_header(Fc *hfc, char *name, int depth);

// Scope
Scope *scope_init(Allocator *alc, int type, Scope *parent, bool has_ast);
void name_taken_check(Fc *fc, Scope *scope, char *name);
Scope *scope_find(Scope *scope, int type);
Scope *scope_find_return_scope(Scope *scope);
bool scope_contains(Scope *parent_scope, Scope *scope);
void scope_apply_issets(Allocator *alc, Scope *scope, Array *issets);
void scope_add_defer_token(Allocator *alc, Scope *scope, Token *token);

// Func
Func *func_init(Allocator *alc, Build *b);
void fcall_type_check(Fc *fc, Value *on, Array *values);
void func_make_arg_decls(Func *func);

// Class
Class *class_init(Allocator *alc);
ClassProp *class_prop_init(Allocator *alc, Class *class, Type *type);
bool class_check_size(Class *class);
Func *class_define_func(Fc *fc, Class *class, bool is_static, char *name, Array *args, Type *rett, int line);
void class_ref_change(Allocator *alc, Scope *scope, Value *on, int amount, bool weak);
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
bool type_is_string(Type *type, Build *b);
Type *type_gen_class(Allocator *alc, Class *class);
Type *type_gen_fptr(Allocator *alc, Func *func);
Type *type_gen_int(Build *b, Allocator *alc, int bytes, bool is_signed);
Type *type_gen_void(Allocator *alc);
Type *type_gen(Build *b, Allocator *alc, char *name);
Type *type_array_of(Allocator *alc, Build *b, Type *type, int size);
Type *read_type(Fc *fc, Allocator *alc, Scope *scope, bool sameline, bool allow_space, int context);
bool type_compat(Type *t1, Type *t2, char **reason);
char *type_to_str(Type *t, char *res, bool simple);
void type_check(Fc *fc, Type *t1, Type *t2);
Type *type_clone(Allocator *alc, Type *type);
bool type_tracks_ownership(Type *type);
bool type_allowed_async(Type *type, bool recursive);
Type *type_get_inline(Allocator *alc, Type *type);
TypeCheck *type_gen_type_check(Allocator *alc, Type *type);
void type_validate(Fc *fc, TypeCheck *check, Type *type, char *msg);
void type_check_to_str(TypeCheck *tc, char *buf);
Type *type_merge(Build* build, Allocator *alc, Type *a, Type *b);

// Var
Decl *decl_init(Allocator *alc, Scope *scope, char *name, Type *type, Value *val, bool is_arg);
Var *var_init(Allocator *alc, Decl *decl, Type *type);
Arg *arg_init(Allocator *alc, char *name, Type *type);

// UsageLine
UsageLine *usage_line_init(Allocator *alc, Scope *scope, Decl *decl);
UsageLine *usage_line_get(Scope *scope, Decl *decl);
bool is_moved_once(UsageLine *ul);
void usage_read_value(Allocator *alc, Scope *scope, Value *val);
Value *usage_move_value(Allocator *alc, Fc *fc, Scope *scope, Value *val, Type* storage_type);
Scope *usage_scope_init(Allocator *alc, Scope *parent, int type);
void usage_merge_ancestors(Allocator *alc, Scope *left, Array *ancestors);
void usage_collect_used_decls(Allocator *alc, Scope *left, Scope *right, Array **list);
void end_usage_line(Allocator *alc, UsageLine *ul, Array *ast);
void deref_scope(Allocator *alc, Scope *scope, Scope *until);
void deref_expired_decls(Allocator *alc, Scope *scope, Array *ast);
void usage_clear_ancestors(Scope *scope);
Scope *usage_create_deref_scope(Allocator *alc, Scope *scope);

// Value
Value *value_init(Allocator *alc, int type, void *item, Type *rett);
Value *read_value(Fc *fc, Allocator *alc, Scope *scope, bool sameline, int prio, bool assignable);
Value *value_op(Fc *fc, Allocator *alc, Scope *scope, Value *left, Value *right, int op);
Value *try_convert(Fc *fc, Allocator *alc, Value *val, Type *to_type);
bool value_is_assignable(Value *v);
void value_disable_upref_deref(Value *val);

Value *vgen_vint(Allocator *alc, long int value, Type *type, bool force_type);
Value *vgen_vfloat(Allocator *alc, Build *b, float value, bool force_type);
Value *vgen_ptrv(Allocator *alc, Value *on, Type *as, Value *index);
Value *vgen_op(Allocator *alc, Build *b, Value *left, Value *right, int op, bool is_ptr);
Value *vgen_compare(Allocator *alc, Build *b, Value *left, Value *right, int op);
Value *vgen_fcall(Allocator *alc, Scope *scope, Value *on, Array *values, Type *rett, FCallOr * or, int line, int col);
Value *vgen_fptr(Allocator *alc, Func *func, Value *first_arg);
Value *vgen_class_pa(Allocator *alc, Scope *scope, Value *on, ClassProp *prop);
Value *vgen_class_init(Allocator *alc, Scope *scope, Class *class, Map *values);
Value *vgen_cast(Allocator *alc, Value *val, Type *to_type);
Value *vgen_null(Allocator *alc, Build *b);
Value *vgen_or_break(Allocator *alc, Value *value, Scope *or_scope, Scope *else_scope, Scope *deref_scope);
Value *vgen_or_value(Allocator *alc, Value *left, Value *right, Scope *value_scope, Scope *else_scope, Scope *deref_scope);
Value *vgen_and_or(Allocator *alc, Build *b, Value *left, Value *right, int op);
Value *vgen_ir_val(Allocator *alc, Value *value, Type *rett);
Value *vgen_ir_from(Allocator *alc, Value *from);
Value *vgen_ir_assign_val(Allocator *alc, Value *value, Type *rett);
Value *vgen_value_and_exec(Allocator *alc, Value *value, Scope *exec_scope, bool before_value, bool enable_exec);
Value *vgen_value_then_ir_value(Allocator *alc, Value *value);
Value *vgen_incr_decr(Allocator *alc, Value *on, bool is_incr);
Value *vgen_atomicop(Allocator *alc, Value *left, Value *right, int op);
Value *vgen_array_item(Allocator *alc, Scope *scope, Value *on, Value *index);
Value *vgen_swap(Allocator *alc, Value *var, Value *with);
Value *vgen_this_or_that(Allocator *alc, Value *cond, Scope *true_scope, Value *left, Scope *false_scope, Value *right, Type *rett);

// Ast
void read_ast(Fc *fc, Scope *scope, bool single_line);

// Token
Token *token_init(Allocator *alc, int type, void *item);
TIf *tgen_tif(Allocator *alc, Value *cond, Scope *scope, Scope *else_scope, Scope *deref_scope);
Token *tgen_declare(Allocator *alc, Decl *decl, Value *val);
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Scope *fscope, Value *retv);
Token *tgen_vscope_return(Allocator *alc, Scope *vscope, Value *retv);
Token *tgen_while(Allocator *alc, Value *cond, Scope *scope);
Token *tgen_exec(Allocator *alc, Scope *scope, bool enable);
Token *tgen_ref_change_exec(Allocator *alc, Scope *scope, Value *on, int amount);
Token *tgen_each(Allocator *alc, Value *value, Scope *scope, Decl *decl_key, Decl *decl_value, int line, int col);
