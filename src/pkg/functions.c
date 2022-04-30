
#include "../all.h"

bool pkg_is_url(char *name) {
    //
    return strchr(name, '.') != NULL;
}

bool pkg_is_github_url(char *name) {
    //
    return starts_with(name, "github.com/");
}

GithubPkg *pkg_parse_github_url(char *name) {
    //
    if (!pkg_is_github_url(name))
        return NULL;
    Array *parts = explode("/", name);
    if (parts->length != 3) {
        return NULL;
    }
    GithubPkg *ghub = malloc(sizeof(GithubPkg));
    ghub->url = name;
    ghub->username = strdup(array_get_index(parts, 1));
    ghub->pkgname = strdup(array_get_index(parts, 2));
    return ghub;
}
