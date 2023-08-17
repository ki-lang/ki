
#include "../all.h"

void *lsp_definition_entry(void *ld_);

cJSON *lsp_definition(Allocator *alc, cJSON *params, int id) {
    //
    cJSON *doc = cJSON_GetObjectItemCaseSensitive(params, "textDocument");
    cJSON *pos = cJSON_GetObjectItemCaseSensitive(params, "position");
    if (pos && doc) {
        cJSON *uri_ = cJSON_GetObjectItemCaseSensitive(doc, "uri");
        cJSON *line_ = cJSON_GetObjectItemCaseSensitive(pos, "line");
        cJSON *col_ = cJSON_GetObjectItemCaseSensitive(pos, "character");
        if (uri_ && uri_->valuestring && line_ && col_) {

            char *uri = strdup(uri_->valuestring);

            if (starts_with(uri, "file://")) {
                int uri_len = strlen(uri);
                memcpy(uri, uri + 7, uri_len - 6);
            }

            char *text = map_get(lsp_doc_content, uri);
            if (text) {
                int line = line_->valueint;
                int col = col_->valueint;

                LspData *ld = lsp_data_init();
                ld->type = lspt_definition;
                ld->id = id;
                ld->line = line;
                ld->col = col;
                ld->filepath = uri;
                ld->index = lsp_get_pos_index(text, line, col);
                ld->text = strdup(text);

#ifdef WIN32
                void *thr = CreateThread(NULL, 0, (unsigned long (*)(void *))lsp_definition_entry, (void *)ld, 0, NULL);
#else
                pthread_t thr;
                pthread_create(&thr, NULL, lsp_definition_entry, (void *)ld);
#endif
                return NULL;
            }
        }
    }

    return cJSON_CreateNull();
}

void lsp_definition_respond(Build *b, LspData *ld, char *path, int line, int col) {
    //
    cJSON *result = cJSON_CreateObject();
    cJSON *range = cJSON_CreateObject();
    cJSON *pos = cJSON_CreateObject();
    cJSON *pos2 = cJSON_CreateObject();

    Allocator *alc = b->alc;
    Str *uri = str_make(alc, 500);
    str_append_chars(uri, "file://");
    str_append_chars(uri, path);

    char *uri_str = str_to_chars(alc, uri);
    lsp_log(uri_str);
    lsp_log("<<<\n");
    cJSON_AddItemToObject(result, "uri", cJSON_CreateString(uri_str));
    cJSON_AddItemToObject(result, "range", range);
    cJSON_AddItemToObject(range, "start", pos);
    cJSON_AddItemToObject(range, "end", pos2);

    cJSON_AddItemToObject(pos, "line", cJSON_CreateNumber(line));
    cJSON_AddItemToObject(pos, "character", cJSON_CreateNumber(col));
    cJSON_AddItemToObject(pos2, "line", cJSON_CreateNumber(line));
    cJSON_AddItemToObject(pos2, "character", cJSON_CreateNumber(col));

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "id", cJSON_CreateNumber(ld->id));
    cJSON_AddItemToObject(resp, "result", result);

    ld->responded = true;
    lsp_respond(resp);
    lsp_exit_thread();
}

void *lsp_definition_entry(void *ld_) {
    //
    LspData *ld = (LspData *)ld_;

    int argc = 3;
    char *argv[argc];
    argv[0] = "ki";
    argv[1] = "build";
    argv[2] = ld->filepath;

    cmd_build(argc, argv, ld);

    lsp_data_free(ld);
    return NULL;
}
