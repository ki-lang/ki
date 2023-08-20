
#include "../all.h"

void *lsp_completion_entry(void *ld);

cJSON *lsp_completion(Allocator *alc, cJSON *params, int id) {
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
                ld->type = lspt_completion;
                ld->id = id;
                ld->line = line;
                ld->col = col;
                ld->filepath = uri;
                ld->index = lsp_get_pos_index(text, line, col);
                ld->text = strdup(text);

                lsp_run_build(ld);
                return NULL;
            }
        }
    }
    return cJSON_CreateNull();
}

void lsp_completion_respond(Allocator *alc, LspData *ld, Array *items) {
    //
    cJSON *result = cJSON_CreateObject();
    cJSON *items_ = cJSON_CreateArray();

    cJSON_AddItemToObject(result, "isIncomplete", cJSON_CreateFalse());
    cJSON_AddItemToObject(result, "items", items_);

    for (int i = 0; i < items->length; i++) {
        LspCompletion *c = array_get_index(items, i);
        cJSON *item = cJSON_CreateObject();
        cJSON_AddItemToObject(item, "kind", cJSON_CreateNumber(c->type));
        cJSON_AddItemToObject(item, "label", cJSON_CreateString(c->label));
        if (c->detail) {
            cJSON *detail = cJSON_CreateObject();
            cJSON_AddItemToObject(item, "labelDetails", detail);
            cJSON_AddItemToObject(detail, "detail", cJSON_CreateString(c->detail));
        }

        if (c->insert) {
            cJSON_AddItemToObject(item, "insertText", cJSON_CreateString(c->insert));
        }
        if (c->type == lsp_compl_method || c->type == lsp_compl_function) {
            cJSON_AddItemToObject(item, "insertTextFormat", cJSON_CreateNumber(2));
        }

        cJSON_AddItemToArray(items_, item);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "id", cJSON_CreateNumber(ld->id));
    cJSON_AddItemToObject(resp, "result", result);

    ld->responded = true;
    lsp_respond(resp);
}
