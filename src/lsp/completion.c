
#include "../all.h"

void *lsp_completion_entry(void *ld);

cJSON *lsp_completion(Allocator *alc, cJSON *params, int id) {
    //
    // cJSON *res = cJSON_CreateObject();
    // cJSON *items = cJSON_CreateArray();

    // cJSON_AddItemToObject(res, "isIncomplete", cJSON_CreateFalse());
    // cJSON_AddItemToObject(res, "items", items);

    cJSON *doc = cJSON_GetObjectItemCaseSensitive(params, "textDocument");
    cJSON *pos = cJSON_GetObjectItemCaseSensitive(params, "position");
    if (doc && pos) {
        cJSON *uri_ = cJSON_GetObjectItemCaseSensitive(doc, "uri");
        cJSON *line_ = cJSON_GetObjectItemCaseSensitive(doc, "line");
        cJSON *col_ = cJSON_GetObjectItemCaseSensitive(doc, "character");
        if (uri_ && uri_->valuestring && line_ && col_) {

            char *uri = strdup(uri_->valuestring);
            char *text = map_get(lsp_doc_content, uri);

            LspData *ld = lsp_data_init();
            ld->type = lspt_completion;
            ld->id = id;
            ld->line = line_->valueint;
            ld->col = col_->valueint;
            ld->filepath = uri;
            ld->text = text;

#ifdef WIN32
            void *thr = CreateThread(NULL, 0, (unsigned long (*)(void *))lsp_completion_entry, (void *)ld, 0, NULL);
#else
            pthread_t thr;
            pthread_create(&thr, NULL, lsp_completion_entry, (void *)ld);
#endif
        }
    }

    return NULL;
}

void *lsp_completion_entry(void *ld_) {
    //
    LspData *ld = (LspData *)ld_;

    char *argv[3];

    lsp_data_free(ld);
    return NULL;
}
