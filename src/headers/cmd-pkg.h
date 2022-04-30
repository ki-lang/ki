
void pkg_help();
void pkg_install(char *name, char *version);
void pkg_remove(char *name);
void pkg_upgrade(char *name, char *version);

bool pkg_is_url(char *name);
bool pkg_is_github_url(char *name);
struct GithubPkg *pkg_parse_github_url(char *name);

typedef struct GithubPkg {
    char *url;
    char *username;
    char *pkgname;
} GithubPkg;
