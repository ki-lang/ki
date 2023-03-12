
#include "../all.h"

void *io_loop(void *build) {
    //
    Build *b = (Build *)build;
    Chain *read_chain = b->read_ki_file;
    Chain *write_chain = b->write_ir;

    while (!b->ir_ready || b->event_count != b->events_done) {
        bool did_work = false;

        Fc *read_fc = chain_get(read_chain);

        if (read_fc) {
            if (b->verbose > 1) {
                printf("👓 Read : %s\n", read_fc->path_ki);
            }
            did_work = true;
            file_get_contents(b->str_buf_io, read_fc->path_ki);
            char *content = str_to_chars(b->alc_io, b->str_buf_io);
            read_fc->chunk->content = content;
            read_fc->chunk->length = strlen(content);
            b->events_done++;
            chain_add(b->stage_1, read_fc);
            continue;
        }

        Fc *write_fc = chain_get(write_chain);

        if (write_fc) {
            did_work = true;
            write_file(write_fc->path_ir, write_fc->ir, false);
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

        if (max_stage > 1) {
            Fc *stage2 = chain_get(b->stage_2);
            while (stage2) {
                stage_2(stage2);
                stage2 = chain_get(b->stage_2);
                did_work = true;
            }
            if (did_work)
                continue;
        }

        if (max_stage > 2) {
            Fc *stage3 = chain_get(b->stage_3);
            while (stage3) {
                stage_3(stage3);
                stage3 = chain_get(b->stage_3);
                did_work = true;
            }
            if (did_work)
                continue;
        }

        if (max_stage > 3) {
            Fc *stage4 = chain_get(b->stage_4);
            while (stage4) {
                stage_4(stage4);
                stage4 = chain_get(b->stage_4);
                did_work = true;
            }
            if (did_work)
                continue;
        }

        if (max_stage > 4) {
            Fc *stage5 = chain_get(b->stage_5);
            while (stage5) {
                stage_5(stage5);
                stage5 = chain_get(b->stage_5);
                did_work = true;
            }
            if (did_work)
                continue;
        }

        if (max_stage > 5) {
            Fc *stage6 = chain_get(b->stage_6);
            while (stage6) {
                stage_6(stage6);
                stage6 = chain_get(b->stage_6);
                did_work = true;
            }
            if (did_work)
                continue;
        }

        if (b->event_count == b->events_done) {
            break;
        }

        sleep_ns(10000); // 10 micro seconds
    }
}
