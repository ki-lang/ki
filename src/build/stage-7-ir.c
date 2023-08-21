
#include "../headers/LLVM.h"

void stage_7(Fc *fc) {
    //
    Build *b = fc->b;
    if (b->verbose > 2) {
        printf("# Stage 7 : LLVM IR : %s\n", fc->path_ki);
    }

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

    char *ir = str_to_chars(b->alc, lb->ir_final);

    char *ir_hash = al(b->alc, 64);
    simple_hash(ir, ir_hash);

    if (strcmp(fc->ir_hash, ir_hash) != 0 || b->clear_cache) {

        fc->ir_hash = ir_hash;
        fc->ir_changed = true;

        if (b->verbose > 1) {
            printf("🧪 Write IR : %s\n", fc->path_ir);
        }

        fc->ir = ir;
        // chain_add(b->write_ir, fc);
        // b->event_count++;
        write_file(fc->path_ir, fc->ir, false);
    }

    //
    alc_wipe(alc);
}

//
