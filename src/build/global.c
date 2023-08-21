
#include "../all.h"

Map *lsp_doc_content;

#ifdef _WIN32
const char PATH_SLASH_CHAR = '\\';
const char *PATH_SLASH = "\\";
HANDLE lsp_resp_lock;
#else
const char PATH_SLASH_CHAR = '/';
const char *PATH_SLASH = "/";
pthread_mutex_t lsp_resp_lock;
#endif
