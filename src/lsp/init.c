
#include "../all.h"

cJSON *lsp_init(Allocator *alc, cJSON *params) {
    //
    cJSON *resp = cJSON_CreateObject();

    cJSON *caps = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "capabilities", caps);

    return resp;
}
