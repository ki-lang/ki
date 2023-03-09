
#include "../all.h"

void *io_loop(void *build) {
    //
    Build *b = (Build *)build;
    Array *read_list = b->queue_read_ki_file;
    Array *write_list = b->queue_write_ir;

    int read_index = 0;
    int write_index = 0;

    while (!b->ir_ready) {
        bool did_work = false;

        if (read_index < read_list->length) {
            did_work = true;
            Fc *fc = array_get_index(read_list, read_index);
            read_index++;
            Str *content_str = file_get_contents(b->alc, fc->path_ki);
            fc->content = str_to_chars(b->alc, content_str);
        }

        if (write_index < write_list->length) {
            did_work = true;
            Fc *fc = array_get_index(write_list, write_index);
            read_index++;
            char *content = str_to_chars(NULL, fc->ir);
            write_file(fc->path_ir, content, false);
            free(content);
        }

        if (did_work)
            sleep_ns(10000); // 10 micro seconds
    }
}

void compile_loop(Build *b) {
    //
}
