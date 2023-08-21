
#include "../all.h"

void *lsp_diagnostic_entry(void *ld_);

cJSON *lsp_save(Allocator *alc, cJSON *params) {
    //
    cJSON *doc = cJSON_GetObjectItemCaseSensitive(params, "textDocument");
    if (doc) {
        cJSON *uri_ = cJSON_GetObjectItemCaseSensitive(doc, "uri");
        if (uri_ && uri_->valuestring) {

            char *uri = strdup(uri_->valuestring);

            if (starts_with(uri, "file://")) {
                int uri_len = strlen(uri);
                memcpy(uri, uri + 7, uri_len - 6);
            }

            LspData *ld = lsp_data_init();
            ld->type = lspt_diagnostic;
            ld->filepath = uri;

            lsp_run_build(ld);
        }
    }

    return NULL;
}

void lsp_diagnostic_respond(Allocator *alc, LspData *ld, Array *errors_) {
    //
    FcError *first = NULL;
    Array *errors = array_make(alc, 10);
    for (int i = 0; i < errors_->length; i++) {
        FcError *err = array_get_index(errors_, i);
        if (strcmp(err->path, ld->filepath) == 0) {
            if (!first) {
                first = err;
            }
            array_push(errors, err);
        }
    }

    cJSON *result = cJSON_CreateObject();
    cJSON *items_ = cJSON_CreateArray();

    Str *uri = str_make(alc, 500);
    str_append_chars(uri, "file://");
    str_append_chars(uri, ld->filepath);

    cJSON_AddItemToObject(result, "uri", cJSON_CreateString(str_to_chars(alc, uri)));
    cJSON_AddItemToObject(result, "diagnostics", items_);

    for (int i = 0; i < errors->length; i++) {
        FcError *err = array_get_index(errors, i);

        cJSON *item = cJSON_CreateObject();
        cJSON *range = cJSON_CreateObject();
        cJSON *pos = cJSON_CreateObject();
        cJSON *pos2 = cJSON_CreateObject();

        cJSON_AddItemToObject(item, "severity", cJSON_CreateNumber(1));
        cJSON_AddItemToObject(item, "message", cJSON_CreateString(err->msg));
        cJSON_AddItemToObject(item, "range", range);

        cJSON_AddItemToObject(range, "start", pos);
        cJSON_AddItemToObject(range, "end", pos2);

        cJSON_AddItemToObject(pos, "line", cJSON_CreateNumber(err->line));
        cJSON_AddItemToObject(pos, "character", cJSON_CreateNumber(err->col));
        cJSON_AddItemToObject(pos2, "line", cJSON_CreateNumber(err->line));
        cJSON_AddItemToObject(pos2, "character", cJSON_CreateNumber(err->col));

        cJSON_AddItemToArray(items_, item);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "method", cJSON_CreateString("textDocument/publishDiagnostics"));
    cJSON_AddItemToObject(resp, "params", result);

    ld->responded = true;
    lsp_respond(resp);
}
