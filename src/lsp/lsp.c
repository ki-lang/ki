
#include "../all.h"

void cmd_lsp_help();
void cmd_lsp_server();
Array *cmd_lsp_parse_input(Allocator *alc, Str *input);

void cmd_lsp(int argc, char *argv[]) {

    Allocator *alc = alc_make();

    Array *args = array_make(alc, argc);
    Map *options = map_make(alc);
    Array *has_value = array_make(alc, 8);

    parse_argv(argv, argc, has_value, args, options);

    if (array_contains(args, "-h", arr_find_str) || array_contains(args, "--help", arr_find_str)) {
        cmd_lsp_help();
    }

    if (args->length < 3) {
        cmd_lsp_help();
    }
    char *action = array_get_index(args, 2);
    if (strcmp(action, "start") == 0) {
        printf("# Start LSP server\n");

        // int argcl = 3;
        // char *argvl[argcl];
        // argvl[0] = "ki";
        // argvl[1] = "build";
        // argvl[2] = "--lsp";

        // cmd_build(argcl, argvl);

        cmd_lsp_server();

    } else {
        cmd_lsp_help();
    }
}

void cmd_lsp_server() {
    //
    Allocator *alc = alc_make();
    Allocator *alc_buf = alc_make();
    Str *input = str_make(alc, 1000);

    while (true) {
        alc_wipe(alc_buf);
        Str *buf = str_make(alc_buf, 1000);
        str_append(buf, input);
        alc_wipe(alc);
        input = str_make(alc, 1000);
        str_append(input, buf);

        int chunk_len = 1000;
        int rcvd = chunk_len;
        char part[chunk_len];
        while (rcvd == chunk_len) {
            rcvd = read(STDIN_FILENO, part, chunk_len);
            if (rcvd > 0) {
                str_append_from_ptr(input, part, rcvd);
            }
        }

        Array *reqs = cmd_lsp_parse_input(alc, input);
        if (!reqs) {
            // Invalid input
            str_clear(input);
            continue;
        }

        for (int i = 0; i < reqs->length; i++) {
            char *req = array_get_index(reqs, i);
            printf("req: '%s'\n", req);

            // cJSON *json = cJSON_ParseWithLength(req, input->length);
            // if (json) {
            //     cJSON *item = cJSON_GetObjectItemCaseSensitive(json, "");
            // }
        }
    }
}

Array *cmd_lsp_parse_input(Allocator *alc, Str *input) {
    //
    Array *res = array_make(alc, 10);

    int len = input->length;
    char *data = input->data;
    int i = 0;
    int start_i = 0;
    int content_len = 0;

    // Requests
    while (i < len) {

        // Headers
        while (i < len) {

            if (data[i] == '\r' && data[i + 1] == '\n') {
                i += 2;
                break;
            }

            Str *key = str_make(alc, 100);
            Str *value = str_make(alc, 100);
            // Key
            while (i < len) {
                char ch = data[i];
                i++;
                if (ch == ' ')
                    continue;
                if (ch == ':')
                    break;
                if (ch == '\r' || ch == '\n')
                    return NULL;
                str_append_char(key, ch);
            }
            if (i >= len)
                break;

            // Skip space
            while (i < len) {
                char ch = data[i];
                i++;
                if (ch == ' ')
                    continue;
            }

            // Value
            while (i < len) {
                char ch = data[i];
                i++;
                if (ch == '\r' && data[i] == '\n') {
                    i++;
                    break;
                }
                str_append_char(value, ch);
            }

            char *key_ = str_to_chars(alc, key);
            if (strcmp(key_, "Content-Length") == 0) {
                char *value_ = str_to_chars(alc, key);
                content_len = atoi(value_);
            }
        }
        if (content_len == 0)
            break;

        // Content
        int count = 0;
        Str *content = str_make(alc, 1000);
        while (i < len && count < content_len) {
            char ch = data[i];
            i++;
            str_append_char(content, ch);
            count++;
        }

        if (count == content_len) {
            char *body = str_to_chars(alc, content);
            array_push(res, body);
            start_i = i;
            continue;
        }
        break;
    }

    // Clear input until start_i
    if (start_i == i) {
        str_clear(input);
    } else {
        void *data = input->data;
        memcpy(data, data + start_i, i - start_i);
    }

    //
    return res;
}

void cmd_lsp_help() {
    //
    printf("> ki lsp start\n");
    printf("\n");

    exit(1);
}
