
#include "../all.h"
#include "../libs/nxjson.h"

PkgCompiler* init_pkc() {
  PkgCompiler* pkc = malloc(sizeof(PkgCompiler));
  pkc->name = NULL;
  pkc->dir = NULL;
  pkc->config = NULL;
  pkc->config_path = NULL;
  pkc->namespaces = map_make();
  pkc->namespace_dirs = map_make();
  pkc->package_dirs = map_make();
  pkc->file_compilers = map_make();
  return pkc;
}

void free_pkc(PkgCompiler* pkc) {
  free(pkc->name);
  free(pkc->dir);
  //
  if (pkc->config) {
    nx_json_free(pkc->config);
  }
  //
  free(pkc);
}

///

PkgCompiler* pkc_get_by_name(char* name) {
  if (map_contains(packages, name)) {
    return map_get(packages, name);
  }

  if (strcmp(name, "ki") == 0) {
    PkgCompiler* pkc = init_pkc();
    pkc->name = "ki";
    map_set(packages, pkc->name, pkc);

    char* dir = malloc(1000);
    strcpy(dir, get_binary_dir());
    strcat(dir, "/lib");
    pkc->dir = dir;
    pkc_check_config(pkc);

    return pkc;
  }

  char* msg = malloc(1024);
  sprintf(msg, "Unknown package: %s", name);
  die(msg);
  return NULL;
}

NsCompiler* pkc_get_namespace_by_name(PkgCompiler* pkc, char* name) {
  NsCompiler* nsc = map_get(pkc->namespaces, name);
  //
  if (nsc == NULL) {
    printf("Unknown namespace: '%s'\n", name);
    exit(1);
  }
  return nsc;
}

NsCompiler* pkc_create_namespace(PkgCompiler* pkc, char* name) {
  NsCompiler* nsc = map_get(pkc->namespaces, name);
  if (nsc == NULL) {
    nsc = init_nsc();
    nsc->name = strdup(name);
    nsc->pkc = pkc;

    // printf("Created namespace: '%s'\n", name);

    map_set(pkc->namespaces, nsc->name, nsc);

    // If pkc has a dir for this namespace, parse all files
    char* dir = map_get(pkc->namespace_dirs, name);
    if (dir != NULL) {
      Array* files = get_subfiles(dir, false, true);
      int i = files->length;
      while (i > 0) {
        i--;
        char* path = malloc(1000);
        char* fn = array_get_index(files, i);
        strcpy(path, dir);
        strcat(path, "/");
        strcat(path, fn);
        if (ends_with(path, ".ki")) {
          fc_new_file(pkc, path, false);
        }
        free(path);
      }
      array_free(files);
    }
  }
  return nsc;
}

void pkc_check_config(PkgCompiler* pkc) {
  //
  char* path = malloc(strlen(pkc->dir) + 20);
  strcpy(path, pkc->dir);
  strcat(path, "/ki.json");
  pkc->config_path = path;

  if (file_exists(path)) {
    Str* content_str = file_get_contents(path);
    char* content = str_to_chars(content_str);
    free(content_str);
    const nx_json* json = nx_json_parse(content, 0);

    pkc->config = json;

    const nx_json* ob = nx_json_get(json, "namespaces");
    if (ob != NULL) {
      nx_json* item;
      for (item = ob->children.first; item; item = item->next) {
        if (item->key && item->type == NX_JSON_STRING) {
          char* dir = malloc(KI_PATH_MAX);
          strcpy(dir, pkc->dir);
          strcat(dir, "/");
          strcat(dir, item->text_value);
          if (!file_exists(dir)) {
            printf("Directory not found:%s\n", dir);
            continue;
          }

          map_set(pkc->namespace_dirs, strdup(item->key), dir);
        }
      }
    }
    free(content);
  }
}