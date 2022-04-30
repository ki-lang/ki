
#include "../all.h"

void pkg_install(char *name, char *version) {
    //
    char *url;
    char *alias;

    if (pkg_is_url(name)) {
        if (pkg_is_github_url(name)) {

            GithubPkg *ghub = pkg_parse_github_url(name);
            if (!ghub) {
                die_token("Invalid github URL: '%s'", name);
            }

            url = ghub->url;
            alias = ghub->pkgname;

            // Get version
            if (strcmp(version, "latest") == 0) {
            }

        } else {
            die_token("Invalid package repository URL: '%s'", name);
        }
    } else {
        die("We currently only support packages via github urls\n");
    }

    //
}
