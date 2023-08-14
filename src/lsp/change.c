
#include "../all.h"

cJSON *lsp_change(Allocator *alc, cJSON *params) {
    //
    cJSON *doc = cJSON_GetObjectItemCaseSensitive(params, "textDocument");
    if (doc) {
        cJSON *uri_ = cJSON_GetObjectItemCaseSensitive(doc, "uri");
        cJSON *changes = cJSON_GetObjectItemCaseSensitive(params, "contentChanges");
        cJSON *text_ = NULL;
        if (changes) {
            cJSON *change = cJSON_GetArrayItem(changes, 0);
            if (change) {
                text_ = cJSON_GetObjectItemCaseSensitive(change, "text");
            }
        }
        if (uri_ && text_) {
            char *uri = uri_->valuestring;
            char *text = text_->valuestring;
            if (uri && text && starts_with(uri, "file://")) {
                int uri_len = strlen(uri);
                char path[uri_len];
                memcpy(path, uri + 6, uri_len - 5);

                char *prev = map_get(lsp_doc_content, path);
                if (prev) {
                    free(prev);
                }
                map_set(lsp_doc_content, path, strdup(text));
            }
        }
    }
    return NULL;
}
