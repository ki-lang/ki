
#include "../all.h"
#include "../libs/httpclient.h"

void pkg_install(char *name, char *version, char *alias) {
    //
    char *hash = NULL;
    char zipurl[1024];
    char cloneurl[1024];
    char *url_buf = malloc(512);

    if (pkg_is_url(name)) {
        if (pkg_is_github_url(name)) {

            GithubPkg *ghub = pkg_parse_github_url(name);
            if (!ghub) {
                die_token("Invalid github URL: '%s'", name);
            }

            if (!alias) {
                alias = ghub->pkgname;
            }

            sprintf(url_buf, "/repos/%s/%s/tags", ghub->username, ghub->pkgname);
            char *resp = request("GET", "api.github.com", url_buf);
            if (!resp) {
                die_token("Failed to request github tags for: '%s'", name);
            }
            char *json_body = strdup(resp);
            const nx_json *json = nx_json_parse(json_body, 0);
            if (!json) {
                die_token("Invalid json response in github tags response for: '%s'", name);
            }
            if (json->children.length == 0) {
                die_token("Github repo has no tags/versions: '%s'", name);
            }

            // Get version
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
                    die_token("No valid tags/versions found (format:x.x.x) for '%s'", name);
                }
            } else {
                Version *fversion = extract_version(version);
                if (fversion) {
                    // Version x.x.x syntax
                    for (int i = 0; i < json->children.length; i++) {
                        const nx_json *v = nx_json_item(json, i);
                        const nx_json *vn = nx_json_get(v, "name");
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
                        hash = get_full_commit_hash(ghub, version);
                    } else {
                        // Invalid syntax
                        die_token("Invalid/unknown version syntax '%s'", version);
                    }
                }
            }
            free(json_body);
            nx_json_free(json);

            sprintf(cloneurl, "git@github.com:%s/%s", ghub->username, ghub->pkgname);
            sprintf(zipurl, "https://%s/archive/%s.zip", name, hash);

        } else {
            die_token("Invalid package repository URL: '%s'", name);
        }
    } else {
        die("We currently only support packages via github urls\n");
    }

    if (!hash) {
        die("No version hash found");
    }

    char *dirname = strdup(name);
    // replace slashes
    int i = 0;
    while (true) {
        char ch = dirname[i];
        if (ch == 0)
            break;
        if (ch == '/')
            dirname[i] = '.';
        i++;
    }

    char *bindir = get_binary_dir();
    char pkgpath[KI_PATH_MAX];
    char versionpath[KI_PATH_MAX];
    char zippath[KI_PATH_MAX];
    strcpy(pkgpath, bindir);
    strcat(pkgpath, "/packages");

    if (!file_exists(pkgpath)) {
        makedir(pkgpath, 0755);
    }

    strcat(pkgpath, "/");
    strcat(pkgpath, dirname);

    if (!file_exists(pkgpath)) {
        makedir(pkgpath, 0755);
    }

    char cmd[KI_PATH_MAX];
    char cmdout[1000];

    strcpy(versionpath, pkgpath);
    strcat(versionpath, "/");
    strcat(versionpath, version);

    if (!file_exists(versionpath)) {
        strcpy(cmd, "cd ");
        strcat(cmd, pkgpath);
        strcat(cmd, " && git clone ");
        strcat(cmd, cloneurl);
        strcat(cmd, " ");
        strcat(cmd, version);

        exec_simple(cmd, cmdout);
        // makedir(pkgpath, 0755);
    }

    strcpy(cmd, "cd ");
    strcat(cmd, versionpath);
    strcat(cmd, " && git checkout ");
    strcat(cmd, hash);

    exec_simple(cmd, cmdout);

    printf("[+] Installed '%s' (%s)\n", name, hash);

    // strcpy(zippath, pkgpath);
    // strcat(zippath, "/pkg-files.zip");

    // if (!file_exists(versionpath)) {
    //     makedir(versionpath, 0755);
    // }

    // download_file(zipurl, zippath);

    // if (!file_exists(zippath)) {
    //     die_token("Failed to download files for '%s'", name);
    // }

    // unzip(zippath, versionpath);

    // printf("Downloaded files for '%s'\n", name);
    //
}
