
#include "../all.h"

void build_help();

// PkgCompiler
PkgCompiler *init_pkc();
void free_pkc(PkgCompiler *pkc);
PkgCompiler *pkc_get_by_name(char *name);
NsCompiler *pkc_get_namespace_by_name(PkgCompiler *pkc, char *name);
bool pkc_namespace_exists(PkgCompiler *pkc, char *name);
NsCompiler *pkc_get_namespace_or_create(PkgCompiler *pkc, char *name);
void pkc_check_config(PkgCompiler *pkc);

// NsCompiler
NsCompiler *init_nsc();
void free_nsc(NsCompiler *nsc);

// FileCompiler
FileCompiler *init_fc();
void free_fc(FileCompiler *fc);
FileCompiler *fc_new_file(PkgCompiler *pkc, char *path, bool is_cmd_arg_file);
void fc_scan_types(FileCompiler *fc);
void fc_include_headers_from(FileCompiler *fc, FileCompiler *from);
LocalVar *fc_localvar(FileCompiler *fc, char *name, Type *type);

// Build
void cmd_build_init_static();
void cmd_build_init_before_build();
void build_cache_checks();
void save_cache();
void fc_scan_values();
void fc_scan_args_and_props(FileCompiler *fc);
void fc_scan_class_props(Class *class);
void fc_scan_class_prop_values(Class *class);
void fc_build_asts();
void fc_build_ast(FileCompiler *fc, Scope *scope);
void fc_define_global(FileCompiler *fc, int type, char *token);
void fc_scan_globals(FileCompiler *fc);
double get_time();
void mark_used_files();
void mark_used(FileCompiler *fc);

// Scope
Scope *init_scope();
void free_scope(Scope *scope);
Scope *init_sub_scope(Scope *parent);
Scope *get_class_scope(Scope *scope);
Scope *get_func_scope(Scope *scope);
Scope *get_loop_scope(Scope *scope);
Scope *get_vscope_scope(Scope *scope);
void scope_remove_local_var_nullable(Scope *scope, LocalVar *lv);

// Token
Token *init_token();
void free_token(Token *token);
void token_each(FileCompiler *fc, Scope *scope);
void token_return(FileCompiler *fc, Scope *scope);
TokenIf *token_if(FileCompiler *fc, Scope *scope, bool is_else, bool has_condition);
void token_ifnull(FileCompiler *fc, Scope *scope);
void token_notnull(FileCompiler *fc, Scope *scope);
void token_while(FileCompiler *fc, Scope *scope);
void token_throw(FileCompiler *fc, Scope *scope);
void token_set_threaded(FileCompiler *fc, Scope *scope);
void token_mutex_init(FileCompiler *fc, Scope *scope);
void token_mutex_lock(FileCompiler *fc, Scope *scope);
void token_mutex_unlock(FileCompiler *fc, Scope *scope);
void token_break(FileCompiler *fc, Scope *scope);
void token_continue(FileCompiler *fc, Scope *scope);
void token_declare(FileCompiler *fc, Scope *scope, Type *left_type);
void token_assign(FileCompiler *fc, Scope *scope, char *sign, Value *value);
void token_free(FileCompiler *fc, Scope *scope);

// Token / Value
OrToken *fc_read_or_token(FileCompiler *fc, Scope *scope, Type *primary_type, char *token, bool on_func_call);
Scope *fc_read_do_else_scope(FileCompiler *fc, Scope *scope, char *token, LocalVar *lv);
ErrorToken *fc_read_error_token(FileCompiler *fc, int errtype, char *token);

// Identifier
Identifier *init_id();
void free_id(Identifier *id);
IdentifierFor *init_idf();
void free_idf(IdentifierFor *idf);
char *create_c_identifier_with_strings(char *package, char *namespace, char *name);
char *fc_create_identifier_global_cname(FileCompiler *fc, Identifier *id);
Identifier *create_identifier(char *package, char *namespace, char *name);
IdentifierFor *idf_find_in_scope(Scope *scope, Identifier *id);
Identifier *fc_read_identifier(FileCompiler *fc, bool readonly, bool sameline, bool allow_space);
IdentifierFor *fc_read_and_get_idf(FileCompiler *fc, Scope *scope, bool readonly, bool sameline, bool allow_space);
Scope *fc_get_identifier_scope(FileCompiler *fc, Scope *scope, Identifier *id);

// Enum
Enum *init_enum();
void free_enum(Enum *enu);
void fc_read_enum_values(FileCompiler *fc, Enum *enu);

// Class
Class *init_class();
void free_class(Class *class);
ClassProp *init_class_prop();
void free_class_prop(ClassProp *prop);
void fc_scan_class(FileCompiler *fc, Class *class);
Class *fc_make_generic_class(Class *class);
Class *fc_get_generic_class(FileCompiler *fc, Class *class, Scope *scope);
char *fc_class_read_generic_unique_id(FileCompiler *fc, Scope *scope);
Class *fc_get_generic_class_by_hash(Class *class, Array *types);
char *types_to_generic_hash(Array *subtypes);

// Trait
Trait *init_trait();
void free_trait(Trait *trait);

// Converter
void fc_scan_converters(FileCompiler* fc);
void fc_scan_converter(FileCompiler *fc, ConverterPos* cvp);
Function *converter_find_func(Converter *cv, Type *from, Type *to);

// Func
Function *init_func();
void free_func(Function *func);
FunctionArg *init_func_arg();
void free_func_arg(FunctionArg *arg);
void fc_scan_func(FileCompiler *fc, Function *func);
void fc_scan_func_args(Function *func);

// Content Chunks
ContentChunk *init_content_chunk();
void free_chunk(ContentChunk *chunk);
ContentChunk *content_chunk_pop(Array *chunks);
ContentChunk *content_chunk_create_for_fc(Array *chunks, FileCompiler *fc);

// Type
Type *init_type();
void free_type(Type *type);
Type *fc_read_type(FileCompiler *fc, Scope *scope);
Type *fc_identifier_to_type(FileCompiler *fc, Identifier *id, Scope *scope);
void fc_type_make_nullable(FileCompiler *fc, Type *t);
bool type_compatible(Type *t1, Type *t2);
void fc_type_compatible(FileCompiler *fc, Type *t1, Type *t2);
Type *fc_create_type_for_enum(Enum *enu);
char *type_to_str(Type *t);
Type *type_generate_generic(Type *type, Type *subtype);

// Value
Value *init_value();
void free_value(Value *v);
Value *fc_read_value(FileCompiler *fc, Scope *scope, bool readonly, bool sameline, bool allow_space);
Value *fc_read_func_call(FileCompiler *fc, Scope *scope, Value *on);
ValueFuncCall *value_generate_func_call(Function *func);

// Read
char fc_get_char(FileCompiler *fc, int offset);
void fc_next_token(FileCompiler *fc, char *token, bool readonly, bool sameline, bool allow_space);
void fc_expect_token(FileCompiler *fc, char *ch, bool readonly, bool sameline, bool allow_space);

// Cache
FcCache *init_fc_cache();
void free_fc_cache(FcCache *c);
void fc_load_cache(FileCompiler *fc);
void fc_save_cache(FileCompiler *fc);
void fc_check_if_modified(FileCompiler *fc);
void fc_depends_on(FileCompiler *fc, FileCompiler *depfc);

// Error
void fc_error(FileCompiler *, char *, char *);
void fc_warn(FileCompiler *fc, char *msg, char *token);
void fc_name_taken(FileCompiler *fc, Map *identifiers, char *name);

// Skips
void fc_skip_string(FileCompiler *fc);
void fc_skip_body(FileCompiler *fc, char *start, char *end, char *alt_end, bool sameline);
void fc_skip_until_char(FileCompiler *fc, char ch);
void fc_skip_comment(FileCompiler *fc);
void fc_skip_type(FileCompiler *fc);
void fc_skip_macro(FileCompiler *fc);
void fc_skip_assign_value(FileCompiler *fc);

// Headers
void fc_read_header_token(FileCompiler *fc);
void fc_read_link_token(FileCompiler *fc);

// Macro
void fc_parse_macro(FileCompiler *fc, Scope *scope, char *token);
bool fc_resolve_macro_if_value(FileCompiler *fc, Scope *scope);

// Write c
void fc_write_c_all();
void fc_write_c_pre(FileCompiler *fc);
void fc_write_c(FileCompiler *fc);
void fc_write_c_predefine_class(FileCompiler *fc, Class *class);
void fc_write_c_class(FileCompiler *fc, Class *class);
void fc_write_c_global(FileCompiler *fc, GlobalVar *gv);
void fc_write_c_enum(FileCompiler *fc, Enum *enu);
void fc_write_c_func(FileCompiler *fc, Function *func);
void fc_write_c_ast(FileCompiler *fc, Scope *scope);
void fc_indent(FileCompiler *fc, Str *append_to);
void fc_write_c_token(FileCompiler *fc, Token *token);
void fc_write_c_value(FileCompiler *fc, Value *value, bool new_value);
void fc_write_c_type(Str *append_to, Type *type, char *varname);
void fc_write_c_if(FileCompiler *fc, TokenIf *ift);
Str *value_buf(FileCompiler *fc);
char *var_buf(FileCompiler *fc);
void deref_local_vars(FileCompiler *fc, Value *retv, Scope *until_scope);
char *fc_write_c_get_allocator(FileCompiler *fc, int size, bool threaded);
void fc_write_c_write_allocator(Str *code, Str *hcode, char *name, char *size, bool threaded);
void fc_write_c_inits();
char *fc_write_c_ort(FileCompiler *fc, OrToken *ort);

// Compile
void compile_all();
void fc_compile_o_file(FileCompiler *fc);
char *get_compiler_path();
void run_cmd(char *cmd);
void wait_cmd();
