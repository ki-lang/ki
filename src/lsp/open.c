
#include "../all.h"
#include <string.h>

cJSON *lsp_open(Allocator *alc, cJSON *params) {
    //
    cJSON *doc = cJSON_GetObjectItemCaseSensitive(params, "textDocument");
    if (doc) {
        cJSON *uri_ = cJSON_GetObjectItemCaseSensitive(doc, "uri");
        cJSON *text_ = cJSON_GetObjectItemCaseSensitive(doc, "text");
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
