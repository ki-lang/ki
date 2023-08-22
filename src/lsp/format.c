
#include "../all.h"

void *lsp_format_entry(void *ld);

cJSON *lsp_format(Allocator *alc, cJSON *params) {
    //
    cJSON *doc = cJSON_GetObjectItemCaseSensitive(params, "textDocument");
    cJSON *options = cJSON_GetObjectItemCaseSensitive(params, "options");
    if (doc && options) {
        cJSON *uri_ = cJSON_GetObjectItemCaseSensitive(doc, "uri");
        if (uri_ && uri_->valuestring) {

            char *uri = strdup(uri_->valuestring);

            int use_tabs = false;
            int spaces = 4;

            cJSON *insertSpaces = cJSON_GetObjectItemCaseSensitive(options, "insertSpaces");
            if (insertSpaces) {
                use_tabs = insertSpaces->valueint ? false : true;
            }
            cJSON *tabSize = cJSON_GetObjectItemCaseSensitive(options, "tabSize");
            if (tabSize) {
                spaces = tabSize->valueint;
            }

            if (starts_with(uri, "file://")) {
                int uri_len = strlen(uri);
                memcpy(uri, uri + 7, uri_len - 6);
            }

            char *text = map_get(lsp_doc_content, uri);
            if (text) {
                char *result = fmt_format(alc, text, use_tabs, spaces);
                if (result) {
                    cJSON *edits = cJSON_CreateArray();

                    cJSON *edit = cJSON_CreateObject();

                    cJSON *range = cJSON_CreateObject();
                    cJSON *pos = cJSON_CreateObject();
                    cJSON *pos2 = cJSON_CreateObject();

                    cJSON_AddItemToObject(edit, "newText", cJSON_CreateString(result));

                    cJSON_AddItemToObject(edit, "range", range);
                    cJSON_AddItemToObject(range, "start", pos);
                    cJSON_AddItemToObject(range, "end", pos2);

                    cJSON_AddItemToObject(pos, "line", cJSON_CreateNumber(0));
                    cJSON_AddItemToObject(pos, "character", cJSON_CreateNumber(0));
                    cJSON_AddItemToObject(pos2, "line", cJSON_CreateNumber(999999));
                    cJSON_AddItemToObject(pos2, "character", cJSON_CreateNumber(0));

                    cJSON_AddItemToArray(edits, edit);

                    return edits;
                }
            }
        }
    }
    return cJSON_CreateNull();
}