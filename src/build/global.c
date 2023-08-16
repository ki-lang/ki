
#include "../all.h"

Map *lsp_doc_content;
char *lsp_tag = "@__KI_LSP_TAG__";
bool lsp_tag_found = false;

#ifdef _WIN32
HANDLE lsp_resp_lock;
#else
pthread_mutex_t lsp_resp_lock;
#endif
