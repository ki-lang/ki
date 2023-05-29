
#include "all.h"
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

bool get_fullpath(char *filepath, char *buf) {
    char *fullpath = NULL;
#ifdef WIN32
    char *res = _fullpath(buf, filepath, KI_PATH_MAX);
#else
    char *res = realpath(filepath, buf);
#endif
    return res != NULL;
}

int file_exists(const char *path) {
#ifdef WIN32
    DWORD dwAttrib = GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES);
#else
    struct stat sb;
    return (stat(path, &sb) == 0);
#endif
}

int dir_exists(const char *path) {
#ifdef WIN32
    DWORD dwAttrib = GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
#endif
}

// e.g. /etc/nginx/nginx.conf -> /etc/nginx/
// e.g. /etc/nginx/ -> /etc/
// e.g. /etc/ -> /
// e.g. / -> /
void get_dir_from_path(char *path, char *buf) {
    strcpy(buf, path);
    int len = strlen(path) - 1;
    while (len > 0) {
        len--;
        if (buf[len] == '/' || buf[len] == '\\') {
            break;
        }
    }
    buf[len + 1] = '\0';
}
void filepath_pop_basename(char *path) {
    int index = strlen(path);
    while (index > 0) {
        index--;
        if (path[index] == '/' || path[index] == '\\') {
            break;
        }
    }
    path[index] = '\0';
}

char *get_path_basename(Allocator *alc, char *path) {
    char *result = dups(alc, path);
    int len = strlen(path) - 1;
    int original_len = len;
    while (len > 0) {
        len--;
        if (result[len] == '/' || result[len] == '\\') {
            len++;
            break;
        }
    }
    int pos = 0;
    while (len <= original_len) {
        result[pos] = path[len];
        len++;
        pos++;
    }
    result[pos] = '\0';
    return result;
}

char *strip_ext(Allocator *alc, char *fn) {
    char *result = dups(alc, fn);
    int len = strlen(fn) - 1;
    while (len > 0) {
        len--;
        if (result[len] == '.') {
            break;
        }
    }
    if (len > 0) {
        result[len] = '\0';
    }
    return result;
}

void makedir(char *dir, int mod) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    mkdir(dir);
    // _mkdir(dir);
#else
    mkdir(dir, mod);
#endif
}

char *g_binary_dir = NULL;
char *get_binary_dir() {
    if (g_binary_dir != NULL) {
        return g_binary_dir;
    }

    char *buf = malloc(KI_PATH_MAX);
    strcpy(buf, "");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    GetModuleFileName(NULL, buf, KI_PATH_MAX);
#elif defined(__APPLE__)
    // Hardcode apple path... no good alternative for now
    // strcpy(buf, "/opt/ki/ki");
    uint32_t size = KI_PATH_MAX;

    if (_NSGetExecutablePath(buf, &size) < 0) {
        fprintf(stderr, "Could not determine macos ki executable path\n");
        exit(1);
    }
    char *full = malloc(KI_PATH_MAX);
    get_fullpath(buf, full);
    free(buf);
    buf = full;
#else
    int len = readlink("/proc/self/exe", buf, KI_PATH_MAX);
    buf[len] = '\0';
#endif

    // Remove executable name
    int i = strlen(buf);
    while (i > 0) {
        i--;
        char ch = buf[i];
        if (ch == '/' || ch == '\\') {
            break;
        }
    }
    buf[i] = '\0';

    g_binary_dir = strdup(buf);
    free(buf);
    return g_binary_dir;
}

char *g_storage_path = NULL;
char *get_storage_path() {
    if (g_storage_path != NULL) {
        return g_storage_path;
    }
    g_storage_path = malloc(KI_PATH_MAX);
    char *homedir;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    homedir = getenv("USERPROFILE");
    const char *kidir = "\\.ki";
#else
    homedir = getenv("HOME");
    const char *kidir = "/.ki";
#endif

    strcpy(g_storage_path, homedir);
    strcat(g_storage_path, kidir);

    if (!file_exists(g_storage_path)) {
        makedir(g_storage_path, 0700);
        printf("Creating storage directory: %s\n", g_storage_path);
    }

    return g_storage_path;
}

// char *g_cache_path = NULL;
// char *get_cache_dir() {
//     if (g_cache_path != NULL) {
//         return g_cache_path;
//     }
//     g_cache_path = malloc(KI_PATH_MAX);
//     char *storage_dir = get_storage_path();
//     strcpy(g_cache_path, storage_dir);
//     strcat(g_cache_path, "/cache");

//     if (!file_exists(g_cache_path)) {
//         makedir(g_cache_path, 0750);
//         printf("Create cache directory: %s\n", g_cache_path);
//     }

//     strcat(g_cache_path, "/");
//     strcat(g_cache_path, g_cache_hash);

//     if (!file_exists(g_cache_path)) {
//         makedir(g_cache_path, 0750);
//         printf("Create cache project directory: %s\n", g_cache_path);
//     }

//     return g_cache_path;
// }

Array *get_subfiles(Allocator *alc, char *dir, bool dirs, bool files) {
    Array *result = array_make(alc, 2);

#ifdef WIN32
    char pattern[KI_PATH_MAX];
    strcpy(pattern, dir);
    strcat(pattern, "/*");
    WIN32_FIND_DATA data;
    HANDLE hFind = FindFirstFile(pattern, &data); // DIRECTORY
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            bool is_dir = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
            bool is_file = !is_dir;
            if (dirs && is_dir) {
                array_push(result, dups(alc, data.cFileName));
            } else if (files && is_file) {
                array_push(result, dups(alc, data.cFileName));
            }
        } while (FindNextFile(hFind, &data));
        FindClose(hFind);
    } else {
        printf("Cant read files from dir: %s\n", dir);
    }
#else
    DIR *d;
    struct dirent *ent;
    if ((d = opendir(dir)) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir(d)) != NULL) {
            char *path = malloc(strlen(dir) + strlen(ent->d_name) + 2);
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            strcpy(path, dir);
            strcat(path, "/");
            strcat(path, ent->d_name);
            // printf("%s\n", path);

            bool is_dir = false;
            bool is_file = false;
#ifdef WIN32
            DWORD dwAttrib = GetFileAttributes(path);
            if (dwAttrib != INVALID_FILE_ATTRIBUTES) {
                is_dir = (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
                is_file = !is_dir;
            }
#else
            struct stat s;
            if (stat(path, &s) == 0) {
                is_dir = (s.st_mode & S_IFDIR) ? true : false;
                is_file = (s.st_mode & S_IFREG) ? true : false;
            }
#endif
            if (dirs && is_dir) {
                array_push(result, dups(alc, ent->d_name));
            } else if (files && is_file) {
                array_push(result, dups(alc, ent->d_name));
            }
            free(path);
        }
        closedir(d);
    } else {
        // could not open directory
        printf("Cant read files from dir: %s\n", dir);
    }
#endif

    return result;
}

int mod_time(char *path) {
#ifdef WIN32
    HANDLE hndl;
    FILETIME ftMod;
    hndl = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hndl == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if (!GetFileTime(hndl, NULL, NULL, &ftMod)) {
        return 0;
    }
    return ftMod.dwLowDateTime + ftMod.dwHighDateTime;
#else
    struct stat attr;
    stat(path, &attr);
    return attr.st_mtime;
#endif
}

void write_file(char *filepath, char *content, bool append) {
    FILE *fp = fopen(filepath, append ? "a" : "w+");
    if (!fp) {
        char msg[KI_PATH_MAX];
        sprintf(msg, "Failed to write file: %s\n", filepath);
        die(msg);
    }
    fputs(content, fp);
    fclose(fp);
}

void file_get_contents(Str *buf, char *path) {
    str_clear(buf);
    char ch;
    FILE *fp = fopen(path, "r");
    while ((ch = fgetc(fp)) != EOF) {
        str_append_char(buf, ch);
    }
    fclose(fp);
}
