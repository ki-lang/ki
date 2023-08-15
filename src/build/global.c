
#include "../all.h"

Map *lsp_doc_content;

#ifdef _WIN32
HANDLE lsp_resp_lock;
#else
pthread_mutex_t lsp_resp_lock;
#endif
