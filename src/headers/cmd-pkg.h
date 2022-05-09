
void pkg_help();
void pkg_install(char *name, char *version, char *alias);
void pkg_remove(char *name);
void pkg_upgrade(char *name, char *version);

bool pkg_is_url(char *name);
bool pkg_is_github_url(char *name);
struct GithubPkg *pkg_parse_github_url(char *name);
struct Version *extract_version(char *content);
int extract_version_next_nr(char *content, int *index, int len);
bool is_higher_version_than(struct Version *new, struct Version *than);
bool is_same_version(struct Version *a, struct Version *b);
char *get_full_commit_hash(struct GithubPkg *ghub, char *shash);
void unzip(char *zippath, char *dir);

typedef struct GithubPkg {
    char *url;
    char *username;
    char *pkgname;
} GithubPkg;

typedef struct Version {
    int v1;
    int v2;
    int v3;
} Version;