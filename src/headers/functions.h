
void die(char *msg);
void parse_argv(char **argv, int argc, Array *has_value, Array *args, Map *options);
bool ends_with(const char *str, const char *suffix);
char *rand_string(char *str, int size);
int atoi(const char *str);
void sleep_ns(unsigned int ns);

// Alloc
Allocator *alc_make();
AllocatorBlock *alc_block_make(AllocatorBlock *prev, AllocatorBlock *next, size_t size);
void *al(Allocator *alc, size_t size);
AllocatorBlock *al_private(Allocator *alc, size_t size);
void free_block(AllocatorBlock *block);
char *dups(Allocator *alc, char *str);

// Build
void cmd_build(int argc, char **argv);

// Chain
Chain *chain_make(Allocator *alc);
Fc *chain_get(Chain *chain);
void chain_add(Chain *chain, Fc *item);

// Loop
void *io_loop(void *build);
void compile_loop(Build *b, int max_stage);

// Fc
Fc *fc_init(Build *b, char *path_ki, Nsc *nsc);

//
void stage_1(Fc *);
void stage_2(Fc *);
void stage_3(Fc *);
void stage_4(Fc *);
void stage_5(Fc *);
void stage_6(Fc *);
void stage_7(Fc *);
