

#include "../all.h"
#include "../libs/httpclient.h"

bool pkg_is_github_url(char *name) {
    //
    return starts_with(name, "github.com/");
}

GithubPkg *github_parse_url(char *name) {
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

char *github_full_commit_hash(struct GithubPkg *ghub, char *shash) {
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

char *github_find_version_hash(GithubPkg *ghub, char *version) {
    //
    char *hash = NULL;
    char url_buf[512];
    //
    sprintf(url_buf, "/repos/%s/%s/tags", ghub->username, ghub->pkgname);
    char *resp = request("GET", "api.github.com", url_buf);
    if (!resp) {
        die_token("Failed to request github tags for: '%s'", ghub->url);
    }
    char *json_body = strdup(resp);
    const nx_json *json = nx_json_parse(json_body, 0);
    if (!json) {
        die_token("Invalid json response in github tags response for: '%s'", ghub->url);
    }
    if (json->children.length == 0) {
        die_token("Github repo has no tags/versions: '%s'", ghub->url);
    }

    // Get version hash
    if (strcmp(version, "latest") == 0) {
        Version *highest = NULL;
        for (int i = 0; i < json->children.length; i++) {
            const nx_json *v = nx_json_item(json, i);
            const nx_json *vn = nx_json_get(v, "name");
            char *vname = vn->text_value;
            Version *ex_version = extract_version(vname);
            if (highest == NULL || is_higher_version_than(ex_version, highest)) {
                highest = ex_version;
                const nx_json *commit = nx_json_get(v, "commit");
                const nx_json *_hash = nx_json_get(commit, "sha");
                const nx_json *_url = nx_json_get(commit, "url");
                hash = strdup(_hash->text_value);
            }
            free(ex_version);
        }
        if (!highest) {
            die_token("No valid tags/versions found (format:x.x.x) for '%s'", ghub->url);
        }
    } else {
        Version *fversion = extract_version(version);
        if (fversion) {
            // Version x.x.x syntax
            for (int i = 0; i < json->children.length; i++) {
                const nx_json *v = nx_json_item(json, i);
                const nx_json *vn = nx_json_get(v, "name");
                if (vn == NULL) {
                    printf("json:%s\n", json_body);
                    die("Invalid json response from github API. Missing version 'name' field.");
                }
                char *vname = vn->text_value;
                Version *ex_version = extract_version(vname);
                if (is_same_version(fversion, ex_version)) {
                    const nx_json *commit = nx_json_get(v, "commit");
                    const nx_json *_hash = nx_json_get(commit, "sha");
                    const nx_json *_url = nx_json_get(commit, "url");
                    hash = strdup(_hash->text_value);
                    free(ex_version);
                    break;
                }
                free(ex_version);
            }
            if (!hash) {
                die_token("Version '%s' not found in the repository tags", version);
            }
        } else {
            // Check if head syntax
            if (is_valid_varname(version)) {
                hash = github_full_commit_hash(ghub, version);
            } else {
                // Invalid syntax
                die_token("Invalid/unknown version syntax '%s'", version);
            }
        }
    }

    //
    free(json_body);
    nx_json_free(json);
    //
    return hash;
}