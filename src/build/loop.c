
#include "../all.h"

void *io_loop(void *build) {
    //
    Build *b = (Build *)build;
    Chain *read_chain = b->read_ki_file;
    Chain *write_chain = b->write_ir;

    while (!b->ir_ready) {
        bool did_work = false;

        Fc *read_fc = chain_get(read_chain);

        if (read_fc) {
            if (b->verbose > 1) {
                printf("👓 Read : %s\n", read_fc->path_ki);
            }
            did_work = true;
            Str *content_str = file_get_contents(b->alc_io, read_fc->path_ki);
            char *content = str_to_chars(b->alc_io, content_str);
            read_fc->chunk->content = content;
            read_fc->chunk->length = strlen(content);
            b->events_done++;
            chain_add(b->stage_1, read_fc);
            continue;
        }

        Fc *write_fc = chain_get(write_chain);

        if (write_fc) {
            did_work = true;
            char *content = str_to_chars(NULL, write_fc->ir);
            write_file(write_fc->path_ir, content, false);
            free(content);
            b->events_done++;
            continue;
        }

        sleep_ns(10000); // 10 micro seconds
    }
}

void compile_loop(Build *b, int max_stage) {
    //

    while (true) {
        bool did_work = false;

        Fc *stage1 = chain_get(b->stage_1);
        while (stage1) {
            stage_1(stage1);
            stage1 = chain_get(b->stage_1);
        }

        if (b->event_count > b->events_done) {
            sleep_ns(5000); // 5 micro seconds
            continue;
        }

        Fc *stage2 = chain_get(b->stage_2);
        while (stage2 && max_stage > 1) {
            stage_2(stage2);
            stage2 = chain_get(b->stage_2);
            did_work = true;
        }
        if (did_work)
            continue;

        Fc *stage3 = chain_get(b->stage_3);
        while (stage3 && max_stage > 2) {
            stage_3(stage3);
            stage3 = chain_get(b->stage_3);
            did_work = true;
        }
        if (did_work)
            continue;

        Fc *stage4 = chain_get(b->stage_4);
        while (stage4 && max_stage > 3) {
            stage_4(stage4);
            stage4 = chain_get(b->stage_4);
            did_work = true;
        }
        if (did_work)
            continue;

        Fc *stage5 = chain_get(b->stage_5);
        while (stage5 && max_stage > 4) {
            stage_5(stage5);
            stage5 = chain_get(b->stage_5);
            did_work = true;
        }
        if (did_work)
            continue;

        Fc *stage6 = chain_get(b->stage_6);
        while (stage6 && max_stage > 5) {
            stage_6(stage6);
            stage6 = chain_get(b->stage_6);
            did_work = true;
        }
        if (did_work)
            continue;

        if (b->event_count == b->events_done) {
            break;
        }

        sleep_ns(10000); // 10 micro seconds
    }
}
