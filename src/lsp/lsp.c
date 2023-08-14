
#include "../all.h"

void cmd_lsp_help();
void cmd_lsp_server();
Array *cmd_lsp_parse_input(Allocator *alc, Str *input);
cJSON *lsp_handle(Allocator *alc, cJSON *json);

void lsp_log(char *msg) {
    //
    if (file_exists("C:/www/out.txt")) {
        write_file("C:/www/out.txt", msg, true);
    } else {
        write_file("/mnt/c/www/out.txt", msg, true);
    }
}

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
        // lsp_log("# Start LSP server\n");

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
    Allocator *alc_keep = alc_make();
    Str *input = str_make(alc, 1000);

    lsp_doc_content = map_make(alc_keep);

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
            // lsp_log("# Wait\n");
            rcvd = read(STDIN_FILENO, part, chunk_len);
            if (rcvd > 0) {
                str_append_from_ptr(input, part, rcvd);
            }
            if (rcvd < 0) {
                // stdin closed
                lsp_log("# Stopping server (stdin closed)\n");
                return;
            }
        }
        if (input->length == 0) {
            lsp_log("# Stopping server (empty input)\n");
            return;
        }
        char *tmp = str_to_chars(alc, input);
        // lsp_log("# Input:\n");
        // lsp_log(tmp);
        // lsp_log("\n");

        Array *reqs = cmd_lsp_parse_input(alc, input);
        if (!reqs) {
            // Invalid input
            lsp_log("# Invalid input\n");
            str_clear(input);
            continue;
        }

        for (int i = 0; i < reqs->length; i++) {
            char *req = array_get_index(reqs, i);
            lsp_log("# Req:\n");
            lsp_log(req);
            lsp_log("\n");

            cJSON *json = cJSON_ParseWithLength(req, strlen(req));
            if (json) {
                cJSON *resp = lsp_handle(alc, json);
                if (resp) {
                    char *str = cJSON_Print(resp);
                    cJSON_Minify(str);
                    int clen = strlen(str);
                    char output[clen + 100];
                    sprintf(output, "Content-Length: %d\r\n\r\n%s", clen, str);
                    write(STDOUT_FILENO, output, strlen(output));

                    lsp_log("# Resp:\n");
                    lsp_log(str);
                    lsp_log("\n");
                    free(str);
                }
            } else {
                lsp_log("# Invalid json\n");
            }
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
    char msg[100];

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
                if (ch == ' ') {
                    i++;
                    continue;
                }
                break;
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
            // sprintf(msg, "header: '%s'\n", key_);
            // lsp_log(msg);
            if (strcmp(key_, "Content-Length") == 0) {
                char *value_ = str_to_chars(alc, value);
                // sprintf(msg, "value: '%s'\n", value_);
                // lsp_log(msg);
                content_len = atoi(value_);
            }
        }
        if (content_len == 0) {
            lsp_log("# No content\n");
            return NULL;
        }

        // Content
        int count = 0;
        Str *content = str_make(alc, 1000);
        while (i < len && count < content_len) {
            char ch = data[i];
            i++;
            str_append_char(content, ch);
            count++;
        }

        // sprintf(msg, "count: %d/%d\n", count, content_len);
        // lsp_log(msg);
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

cJSON *lsp_handle(Allocator *alc, cJSON *json) {
    //
    cJSON *resp = NULL;
    cJSON *error = NULL;

    cJSON *id = cJSON_GetObjectItemCaseSensitive(json, "id");
    cJSON *method = cJSON_GetObjectItemCaseSensitive(json, "method");

    bool respond = true;

    if (method) {
        cJSON *params = cJSON_GetObjectItemCaseSensitive(json, "params");

        if (strcmp(method->valuestring, "initialize") == 0 && params) {
            resp = lsp_init(alc, params);
        } else if (strcmp(method->valuestring, "shutdown") == 0) {
            resp = cJSON_CreateNull();
        } else if (strcmp(method->valuestring, "exit") == 0) {
            exit(0);
        } else if (strcmp(method->valuestring, "textDocument/didOpen") == 0) {
            resp = lsp_open(alc, params);
        } else if (strcmp(method->valuestring, "textDocument/didClose") == 0) {
            resp = lsp_close(alc, params);
        } else if (strcmp(method->valuestring, "textDocument/didChange") == 0) {
            resp = lsp_change(alc, params);
        } else if (strcmp(method->valuestring, "textDocument/definition") == 0) {
            resp = lsp_definition(alc, params);
        } else if (strcmp(method->valuestring, "textDocument/completion") == 0) {
            resp = lsp_completion(alc, params, id->valueint);
            respond = false;
        }
    }
    if (respond && id) {
        cJSON *r = cJSON_CreateObject();
        // cJSON_AddItemToObject(r, "jsonrpc", cJSON_CreateString("2.0"));
        cJSON_AddItemToObject(r, "id", cJSON_CreateNumber(id->valueint));
        if (resp)
            cJSON_AddItemToObject(r, "result", resp);
        if (error)
            cJSON_AddItemToObject(r, "error", error);
        resp = r;
    }

    return resp;
}

LspData *lsp_data_init() {
    //
    LspData *ld = malloc(sizeof(LspData));
    ld->type = 0;
    ld->id = 0;
    ld->line = 0;
    ld->col = 0;
    ld->filepath = NULL;
    ld->text = NULL;
    return ld;
}
void lsp_data_free(LspData *ld) {
    //
    if (ld->filepath)
        free(ld->filepath);
    if (ld->text)
        free(ld->text);
    free(ld);
}

void cmd_lsp_help() {
    //
    printf("> ki lsp start\n");
    printf("\n");

    exit(1);
}
