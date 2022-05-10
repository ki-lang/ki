
#include "../all.h"

void cfg_init() {
    //
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    Config *cfg = cfg_get(cwd);
    if (cfg == NULL) {
        // Create config
        char *path = malloc(strlen(cwd) + 20);
        strcpy(path, cwd);
        strcat(path, "/ki.json");

        cJSON *json = cJSON_CreateObject();
        char *content = cJSON_Print(json);

        write_file(path, content, false);

        cJSON_Delete(json);

        printf("# Your ki.json config has been created\nPath: %s\n", path);
    } else {
        printf("# ki.json already exists\n");
    }
}
