
#include "../all.h"

cJSON *lsp_completion(Allocator *alc, cJSON *params) {
    //
    cJSON *res = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();

    cJSON_AddItemToObject(res, "isIncomplete", cJSON_CreateFalse());
    cJSON_AddItemToObject(res, "items", items);

    return res;
}
