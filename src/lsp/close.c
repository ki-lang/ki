
#include "../all.h"

cJSON *lsp_close(Allocator *alc, cJSON *params) {
    //
    cJSON *doc = cJSON_GetObjectItemCaseSensitive(params, "textDocument");
    if (doc) {
        cJSON *uri_ = cJSON_GetObjectItemCaseSensitive(doc, "uri");
        if (uri_) {
            char *uri = uri_->valuestring;
            if (uri && starts_with(uri, "file://")) {
                int uri_len = strlen(uri);
                char path[uri_len];
                memcpy(path, uri + 6, uri_len - 5);

                char *prev = map_get(lsp_doc_content, path);
                if (prev) {
                    free(prev);
                }
                map_unset(lsp_doc_content, path);
            }
        }
    }
    return NULL;
}
