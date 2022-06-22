
#include "../all.h"

void build_cache_checks() {
    // Check if dependencies have changed
    for (int i = 0; i < g_fc_by_ki_filepath->values->length; i++) {
        FileCompiler *fc = array_get_index(g_fc_by_ki_filepath->values, i);
        fc_check_if_modified(fc);
    }
    // Load uses from cache if file was not modified
}

void save_cache() {
    //
    for (int i = 0; i < g_fc_by_ki_filepath->values->length; i++) {
        FileCompiler *fc = array_get_index(g_fc_by_ki_filepath->values, i);
        fc_save_cache(fc);
    }
}
