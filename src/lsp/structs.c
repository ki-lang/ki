
#include "../all.h"

LspData *lsp_data_init() {
    //
    LspData *ld = malloc(sizeof(LspData));
    ld->type = 0;
    ld->id = 0;
    ld->line = 0;
    ld->col = 0;
    ld->index = -1;
    ld->filepath = NULL;
    ld->text = NULL;
    ld->responded = false;
    ld->send_default = false;
    return ld;
}
void lsp_data_free(LspData *ld) {
    //
    if (ld->filepath)
        free(ld->filepath);
    if (ld->text)
        free(ld->text);
    free(ld);
}

LspCompletion *lsp_completion_init(Allocator *alc, int type, char *label) {
    //
    LspCompletion *c = al(alc, sizeof(LspData));
    c->type = type;
    c->label = label;
    c->detail = NULL;
    c->insert = NULL;

    return c;
}
