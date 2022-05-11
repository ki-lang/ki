
void pkg_help();
void pkg_add(char *name, char *version, char *alias);
void pkg_remove(char *name);
void pkg_install();
void pkg_upgrade(char *name, char *version);

// Structs
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

// Add
bool pkg_is_url(char *name);
bool pkg_is_github_url(char *name);
// Install
void pkg_install_package(char *dir, char *name, char *version, char *clone_url, char *hash);
// Functions
struct Version *extract_version(char *content);
int extract_version_next_nr(char *content, int *index, int len);
bool is_higher_version_than(struct Version *new, struct Version *than);
bool is_same_version(struct Version *a, struct Version *b);
void unzip(char *zippath, char *dir);
// Github
char *github_find_version_hash(GithubPkg *ghub, char *version);
GithubPkg *github_parse_url(char *name);
char *github_full_commit_hash(GithubPkg *ghub, char *shash);
