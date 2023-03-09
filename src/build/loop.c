
#include "../all.h"

void *io_loop(void *build) {
    //
    Build *b = (Build *)build;
    Chain *read_chain = b->read_ki_file;
    Chain *write_chain = b->write_ir;

    int read_index = 0;
    int write_index = 0;
    Fc *last_read_fc = NULL;
    Fc *last_write_fc = NULL;
    Fc *read_fc = NULL;
    Fc *write_fc = NULL;

    while (!b->ir_ready) {
        bool did_work = false;

        if (last_read_fc == NULL) {
            last_read_fc = read_chain->first;
            read_fc = last_read_fc;
        } else {
            read_fc = last_read_fc->next;
        }
        if (write_fc == NULL) {
            last_write_fc = write_chain->first;
            write_fc = last_write_fc;
        } else {
            write_fc = last_write_fc->next;
        }

        if (read_fc) {
            printf("Read: %s\n", read_fc->path_ki);
            did_work = true;
            read_index++;
            Str *content_str = file_get_contents(b->alc, read_fc->path_ki);
            read_fc->content = str_to_chars(b->alc, content_str);
            b->events_done++;
            last_read_fc = read_fc;
        }

        if (write_fc) {
            did_work = true;
            read_index++;
            char *content = str_to_chars(NULL, write_fc->ir);
            write_file(write_fc->path_ir, content, false);
            free(content);
            b->events_done++;
            last_write_fc = write_fc;
        }

        if (did_work)
            sleep_ns(10000); // 10 micro seconds
    }
}

void compile_loop(Build *b) {
    //
    while (true) {
        bool did_work = false;

        if (!did_work) {
            if (b->event_count == b->events_done) {
                break;
            }
            sleep_ns(10000); // 10 micro seconds
        }
    }
}
