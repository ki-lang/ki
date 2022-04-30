
#include "../all.h"
#include "../libs/httpclient.h"

void pkg_install(char *name, char *version_target, char *alias) {
    //
    char *url;
    char *version;

    char *url_buf = malloc(512);

    if (pkg_is_url(name)) {
        if (pkg_is_github_url(name)) {

            GithubPkg *ghub = pkg_parse_github_url(name);
            if (!ghub) {
                die_token("Invalid github URL: '%s'", name);
            }

            url = ghub->url;
            alias = ghub->pkgname;

            // Get version
            if (strcmp(version_target, "latest") == 0) {
                sprintf(url_buf, "/repos/%s/%s/tags", ghub->username, ghub->pkgname);
                char *resp = request("GET", "api.github.com", url_buf);
                printf("result: %s", resp);
            }

        } else {
            die_token("Invalid package repository URL: '%s'", name);
        }
    } else {
        die("We currently only support packages via github urls\n");
    }

    //
}
