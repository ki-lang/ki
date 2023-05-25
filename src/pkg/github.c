
#include "../all.h"
#include "../libs/httpclient.h"

bool pkg_is_github_url(char *name) {
    //
    return starts_with(name, "github.com/");
}

GithubPkg *github_parse_url(Allocator *alc, char *name) {
    //
    if (!pkg_is_github_url(name))
        return NULL;
    Array *parts = explode(alc, "/", name);
    if (parts->length != 3) {
        return NULL;
    }
    GithubPkg *ghub = malloc(sizeof(GithubPkg));
    ghub->url = name;
    ghub->username = strdup(array_get_index(parts, 1));
    ghub->pkgname = strdup(array_get_index(parts, 2));
    return ghub;
}

char *github_full_commit_hash(struct GithubPkg *ghub, char *shash) {
    //
    char msg[256];
    char url_buf[512];
    sprintf(url_buf, "/repos/%s/%s/commits/%s", ghub->username, ghub->pkgname, shash);
    char *resp = request("GET", "api.github.com", url_buf);
    if (!resp) {
        sprintf(msg, "Github API request failed when trying to find full commit hash for: '%s'", shash);
        die(msg);
    }
    char *json_body = resp;
    cJSON *json = cJSON_ParseWithLength(json_body, strlen(json_body));
    if (!json) {
        sprintf(msg, "Invalid json response for github API request: '%s'", url_buf);
        die(msg);
    }
    cJSON *sha = cJSON_GetObjectItemCaseSensitive(json, "sha");
    if (sha == NULL) {
        return NULL;
    }
    char *hash = sha->valuestring;
    return strdup(hash);
}

char *github_find_version_hash(GithubPkg *ghub, char *version) {
    //
    char *hash = NULL;
    char msg[256];
    char url_buf[512];
    //
    sprintf(url_buf, "/repos/%s/%s/tags", ghub->username, ghub->pkgname);
    char *resp = request("GET", "api.github.com", url_buf);
    if (!resp) {
        sprintf(msg, "Failed to request github tags for: '%s'", ghub->url);
        die(msg);
    }
    char *json_body = strdup(resp);
    cJSON *json = cJSON_ParseWithLength(json_body, strlen(json_body));
    if (!json) {
        sprintf(msg, "Invalid json response in github tags response for: '%s'", ghub->url);
        die(msg);
    }
    int itemc = cJSON_GetArraySize(json);
    if (itemc == 0) {
        sprintf(msg, "Github repo has no tags/versions: '%s'", ghub->url);
        die(msg);
    }

    // Get version hash
    if (strcmp(version, "latest") == 0) {
        PkgVersion *highest = NULL;
        for (int i = 0; i < itemc; i++) {
            cJSON *item = cJSON_GetArrayItem(json, i);
            cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
            if (!name)
                continue;
            char *vname = name->valuestring;
            PkgVersion *ex_version = extract_version(vname);
            if (highest == NULL || is_higher_version_than(ex_version, highest)) {
                highest = ex_version;
                cJSON *commit = cJSON_GetObjectItemCaseSensitive(item, "commit");
                if (commit) {
                    cJSON *sha = cJSON_GetObjectItemCaseSensitive(commit, "sha");
                    if (sha) {
                        hash = strdup(sha->valuestring);
                    }
                }
            }
            free(ex_version);
        }
        if (!highest || !hash) {
            sprintf(msg, "No valid tags/versions found (format:x.x.x) for '%s'", ghub->url);
            die(msg);
        }
    } else {
        PkgVersion *fversion = extract_version(version);
        if (fversion) {
            // Version x.x.x syntax
            for (int i = 0; i < itemc; i++) {
                cJSON *item = cJSON_GetArrayItem(json, i);
                cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
                if (name == NULL) {
                    die("Invalid json response from github API. Missing version 'name' field.");
                }
                char *vname = name->valuestring;
                PkgVersion *ex_version = extract_version(vname);
                if (is_same_version(fversion, ex_version)) {
                    cJSON *commit = cJSON_GetObjectItemCaseSensitive(item, "commit");
                    if (commit) {
                        cJSON *sha = cJSON_GetObjectItemCaseSensitive(commit, "sha");
                        if (sha) {
                            hash = strdup(sha->valuestring);
                            free(ex_version);
                            break;
                        }
                    }
                }
                free(ex_version);
            }
            if (!hash) {
                sprintf(msg, "Version '%s' not found in the repository tags", version);
                die(msg);
            }
        } else {
            // Check if commit syntax
            if (is_valid_varname(version)) {
                hash = github_full_commit_hash(ghub, version);
                if (!hash) {
                    sprintf(msg, "Package version/git-hash not found: '%s'", version);
                    die(msg);
                }
            } else {
                // Invalid syntax
                sprintf(msg, "Invalid/unknown version syntax '%s'", version);
                die(msg);
            }
        }
    }

    //
    free(json_body);
    cJSON_free(json);
    //
    return hash;
}
