
#include "../headers/LLVM.h"

void stage_4_2(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 4.2 : LLVM IR : %s\n", fc->path_ki);
    }

    unsigned long start = microtime();

    Allocator *alc = fc->alc_ast;

    LB *lb = al(alc, sizeof(LB));
    lb->build = b;
    lb->fc = fc;
    lb->alc = alc;

    lb->lfunc = NULL;
    lb->lfuncs = array_make(alc, fc->funcs->length + 1);
    lb->defined_classes = array_make(alc, fc->classes->length + 1);
    lb->declared_funcs = array_make(alc, fc->funcs->length + 1);
    lb->globals = map_make(alc);

    lb->char_buf = al(alc, 100);
    lb->str_buf = str_make(alc, 1000);
    lb->str_buf_di_type = str_make(alc, 256);

    lb->ir_final = str_make(alc, 10000);
    lb->ir_struct = str_make(alc, 2000);
    lb->ir_global = str_make(alc, 2000);
    lb->ir_extern_func = str_make(alc, 1000);
    lb->ir_attr = str_make(alc, 1000);

    lb->strc = 0;
    lb->attrc = 1;
    lb->while_cond = NULL;
    lb->while_after = NULL;

    lb->loop_attr = NULL;
    lb->loop_attr_root = NULL;
    lb->attrs = array_make(alc, 10);

    lb->use_stack_save = false;
    lb->debug = b->debug;
    lb->di_cu = NULL;
    lb->di_file = NULL;
    lb->di_retained_nodes = NULL;
    lb->di_type_ptr = NULL;

    llvm_build_ir(lb);

    b->time_ir += microtime() - start;

    str_append_char(lb->ir_final, 0);
    char *ir = lb->ir_final->data;

    char *ir_hash = al(b->alc, 64);
    ctxhash(ir, ir_hash);

    if (strcmp(fc->ir_hash, ir_hash) != 0 || b->clear_cache) {

        fc->ir_hash = ir_hash;
        fc->ir_changed = true;

        if (b->verbose > 1) {
            printf("🧪 Write IR : %s\n", fc->path_ir);
        }

        fc->ir = ir;

        start = microtime();
        write_file(fc->path_ir, fc->ir, false);
        b->time_fs += microtime() - start;
    }

    //
    alc_wipe(alc);
}

//
