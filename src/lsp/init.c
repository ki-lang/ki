
#include "../all.h"

cJSON *lsp_init(Allocator *alc, cJSON *params) {
    //
    cJSON *resp = cJSON_CreateObject();

    cJSON *caps = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "capabilities", caps);

    cJSON *sync = cJSON_CreateObject();
    cJSON_AddItemToObject(caps, "textDocumentSync", sync);

    cJSON_AddItemToObject(sync, "openClose", cJSON_CreateTrue());
    cJSON_AddItemToObject(sync, "change", cJSON_CreateNumber(1)); // Send full content
    cJSON_AddItemToObject(sync, "save", cJSON_CreateNumber(1));   // Send full content

    cJSON_AddItemToObject(caps, "definitionProvider", cJSON_CreateTrue());

    cJSON *compl = cJSON_CreateObject();
    cJSON_AddItemToObject(caps, "completionProvider", compl );
    cJSON *chars = cJSON_CreateArray();
    cJSON_AddItemToArray(chars, cJSON_CreateString("."));
    cJSON_AddItemToArray(chars, cJSON_CreateString(":"));
    cJSON_AddItemToObject(compl, "triggerCharacters", chars);

    cJSON *sighelp = cJSON_CreateObject();
    cJSON_AddItemToObject(caps, "signatureHelpProvider", sighelp);
    cJSON *help_chars = cJSON_CreateArray();
    cJSON_AddItemToArray(help_chars, cJSON_CreateString("("));
    cJSON_AddItemToArray(help_chars, cJSON_CreateString(","));
    cJSON_AddItemToObject(sighelp, "triggerCharacters", help_chars);
    cJSON *help_chars_re = cJSON_CreateArray();
    cJSON_AddItemToArray(help_chars_re, cJSON_CreateString(" "));
    cJSON_AddItemToObject(sighelp, "retriggerCharacters", help_chars_re);

    // cJSON *dia = cJSON_CreateObject();
    // cJSON_AddItemToObject(caps, "diagnosticProvider", dia);
    // cJSON_AddItemToObject(dia, "interFileDependencies", cJSON_CreateTrue());
    // cJSON_AddItemToObject(dia, "workspaceDiagnostics", cJSON_CreateTrue());

    return resp;
}
