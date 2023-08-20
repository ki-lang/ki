
#include "../all.h"

void *lsp_help_entry(void *ld);

cJSON *lsp_help(Allocator *alc, cJSON *params, int id) {
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
                ld->type = lspt_sig_help;
                ld->id = id;
                ld->line = line;
                ld->col = col;
                ld->filepath = uri;
                ld->index = lsp_get_pos_index(text, line, col);
                ld->text = strdup(text);

                lsp_run_build(ld);
                return NULL;
            }
            free(uri);
        }
    }
    return cJSON_CreateNull();
}

void lsp_help_check_args(Allocator *alc, Fc *fc, Array *args, bool skip_first, Type *rett, int arg_index) {
    //
    Build *b = fc->b;
    LspData *ld = b->lsp;
    Str *label = str_make(alc, 50);
    str_append_chars(label, "fn (");

    Array *items = array_make(b->alc, 100);
    int count = 0;
    char part[256];
    for (int i = 0; i < args->length; i++) {
        if (i == 0 && skip_first)
            continue;
        if (count > 0) {
            str_append_chars(label, ", ");
        }
        count++;
        int start = label->length;
        Arg *arg = array_get_index(args, i);
        type_to_str(arg->type, part, true);
        str_append_chars(label, part);
        int end = label->length;

        cJSON *bounds = cJSON_CreateArray();
        cJSON_AddItemToArray(bounds, cJSON_CreateNumber(start));
        cJSON_AddItemToArray(bounds, cJSON_CreateNumber(end));

        array_push(items, bounds);
    }
    str_append_chars(label, ")");
    if (rett) {
        str_append_chars(label, " ");
        type_to_str(rett, part, true);
        str_append_chars(label, part);
    }
    char *full = str_to_chars(alc, label);
    lsp_help_respond(b, ld, full, items, arg_index - (skip_first ? 1 : 0));
}

void lsp_help_respond(Build *b, LspData *ld, char *full, Array *args, int arg_index) {
    //
    cJSON *result = cJSON_CreateObject();
    cJSON *sigs = cJSON_CreateArray();

    cJSON_AddItemToObject(result, "signatures", sigs);
    cJSON_AddItemToObject(result, "activeSignature", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(result, "activeParameter", cJSON_CreateNumber(arg_index));

    cJSON *sig = cJSON_CreateObject();
    cJSON *parts = cJSON_CreateArray();
    cJSON_AddItemToArray(sigs, sig);

    cJSON_AddItemToObject(sig, "label", cJSON_CreateString(full));
    cJSON_AddItemToObject(sig, "parameters", parts);
    cJSON_AddItemToObject(sig, "activeParameter", cJSON_CreateNumber(arg_index));

    for (int i = 0; i < args->length; i++) {
        cJSON *bounds = array_get_index(args, i);
        cJSON *part = cJSON_CreateObject();
        cJSON_AddItemToArray(parts, part);
        cJSON_AddItemToObject(part, "label", bounds);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "id", cJSON_CreateNumber(ld->id));
    cJSON_AddItemToObject(resp, "result", result);

    ld->responded = true;
    lsp_respond(resp);
}
