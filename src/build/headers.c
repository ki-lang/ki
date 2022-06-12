
#include "../all.h"
#include "../libs/nxjson.h"

void fc_read_header_token(FileCompiler *fc) {
    fc_expect_token(fc, "\"", false, true, true);
    char *path = malloc(KI_PATH_MAX);
    int pos = 0;
    char ch = fc_get_char(fc, 0);
    while (ch != '"') {
        if (is_newline(ch)) {
            fc_error(fc, "Unexpected newline", NULL);
        }
        path[pos] = ch;
        pos++;
        fc->i++;
        ch = fc_get_char(fc, 0);
    }
    path[pos++] = '.';
    path[pos++] = 'k';
    path[pos++] = 'h';
    path[pos] = '\0';
    fc->i++;

    PkgCompiler *pkc = fc->nsc->pkc;
    // Check if already imported
    FileCompiler *hfc = map_get(pkc->headers, path);
    if (hfc) {
        fc_include_headers_from(fc, hfc);
        return;
    }

    // Get full path
    char *fullpath = NULL;
    if (pkc->config) {
        const nx_json *headers = nx_json_get(pkc->config, "headers");
        if (headers) {
            const nx_json *ob = nx_json_get(headers, "dirs");
            if (ob != NULL) {
                nx_json *item;
                for (item = ob->children.first; item; item = item->next) {
                    char *find = malloc(KI_PATH_MAX);
                    strcpy(find, pkc->dir);
                    strcat(find, "/");
                    strcat(find, item->text_value);
                    strcat(find, "/");
                    strcat(find, path);
                    //   printf("try:%s\n", find);
                    if (file_exists(find)) {
                        fullpath = find;
                        break;
                    }
                }
            }
        }
    }

    if (!fullpath) {
        fc_error(fc, "Cannot find header file location: '%s'", path);
    }

    free(path);
    // Parse
    PkgCompiler *mainpkc = map_get(packages, "main");
    hfc = fc_new_file(mainpkc, fullpath, false);

    map_set(pkc->headers, path, hfc);
    fc_include_headers_from(fc, hfc);
}

void fc_read_link_token(FileCompiler *fc) {
    //
    if (!fc->is_header) {
        fc_error(fc, "Adding libraries to the linker is only allowed in header files", NULL);
    }
    fc_expect_token(fc, "\"", false, true, true);
    char *name = malloc(KI_PATH_MAX);
    int pos = 0;
    char ch = fc_get_char(fc, 0);
    while (ch != '"') {
        if (is_newline(ch)) {
            fc_error(fc, "Unexpected newline", NULL);
        }
        name[pos] = ch;
        pos++;
        fc->i++;
        ch = fc_get_char(fc, 0);
    }
    name[pos] = '\0';

    fc_expect_token(fc, "\"", false, true, false);

    if (array_find(g_links, name, "chars") == -1) {
        array_push(g_links, strdup(name));
    }
    free(name);
}