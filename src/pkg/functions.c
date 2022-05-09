
#include "../all.h"
#include "../libs/httpclient.h"

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

char *get_full_commit_hash(struct GithubPkg *ghub, char *shash) {
    //
    char url_buf[512];
    sprintf(url_buf, "/repos/%s/%s/commits/%s", ghub->username, ghub->pkgname, shash);
    char *resp = request("GET", "api.github.com", url_buf);
    if (!resp) {
        die_token("Github API request failed when trying to find full commit hash for: '%s'", shash);
    }
    char *json_body = resp;
    const nx_json *json = nx_json_parse(json_body, 0);
    if (!json) {
        die_token("Invalid json response for github API request: '%s'", url_buf);
    }
    const nx_json *sha = nx_json_get(json, "sha");
    if (sha == NULL) {
        die_token("Full hash not found for: '%s'", shash);
    }
    char *hash = sha->text_value;
    return strdup(hash);
}

bool is_higher_version_than(Version *new, Version *than) {
    //
    if (new->v1 != than->v1) {
        return new->v1 > than->v1;
    }
    if (new->v2 != than->v2) {
        return new->v2 > than->v2;
    }
    if (new->v3 != than->v3) {
        return new->v3 > than->v3;
    }
    return false;
}

bool is_same_version(struct Version *a, struct Version *b) {
    //
    return a->v1 == b->v1 && a->v2 == b->v2 && a->v3 == b->v3;
}

Version *extract_version(char *content) {
    //
    // printf("v:%s\n", content);
    int i = 0;
    int len = strlen(content);
    if (starts_with(content, "v")) {
        i++;
    }
    int v1 = extract_version_next_nr(content, &i, len);
    if (v1 == -1)
        return NULL;
    i++;
    int v2 = extract_version_next_nr(content, &i, len);
    if (v2 == -1)
        return NULL;
    i++;
    int v3 = extract_version_next_nr(content, &i, len);
    if (v3 == -1)
        return NULL;
    Version *v = malloc(sizeof(Version));
    v->v1 = v1;
    v->v2 = v2;
    v->v3 = v3;
    return v;
}

int extract_version_next_nr(char *content, int *index, int len) {
    //
    int end = *index;
    int pos = 0;
    char nr[10];
    while (end < len) {
        char ch = content[end];
        if (!is_number(ch))
            break;
        end++;
        nr[pos] = ch;
        pos++;
    }
    nr[pos] = '\0';
    if (pos == 0) {
        return -1;
    }
    *index = end;
    return atoi(nr);
}

#include "../libs/zip/zip.h"

int on_extract_entry(const char *filename, void *arg) {
    static int i = 0;
    int n = *(int *)arg;
    printf("Extracted: %s (%d of %d)\n", filename, ++i, n);
    return 0;
}

void unzip(char *zippath, char *dir) {
    int arg = 2;
    zip_extract(zippath, dir, on_extract_entry, &arg);
}
