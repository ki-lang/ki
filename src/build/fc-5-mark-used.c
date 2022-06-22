
#include "../all.h"

Array *use_dupes;
void mark_used_files() {
    //
    use_dupes = array_make(40);
    //
    Array *fcs = g_fc_by_ki_filepath->values;
    for (int i = 0; i < fcs->length; i++) {
        FileCompiler *fc = array_get_index(fcs, i);
        if (fc->is_used) {
            mark_used(fc);
        }
    }
    //
    if (g_main_func) {
        mark_used(g_main_func->fc);
    }
}

void mark_used(FileCompiler *fc) {

    if (array_contains(use_dupes, fc, "address")) {
        return;
    }
    array_push(use_dupes, fc);

    fc->is_used = true;

    Array *deps = fc->cache->depends_on->keys;
    for (int o = 0; o < deps->length; o++) {
        char *fpath = array_get_index(deps, o);
        FileCompiler *fcd = map_get(g_fc_by_ki_filepath, fpath);
        if (fcd) {
            mark_used(fcd);
        }
    }
}
