
#include "../all.h"

int extract_version_next_nr(char *content, int *index, int len);

bool is_higher_version_than(PkgVersion *new, PkgVersion *than) {
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

bool is_same_version(PkgVersion *a, PkgVersion *b) {
    //
    return a->v1 == b->v1 && a->v2 == b->v2 && a->v3 == b->v3;
}

PkgVersion *extract_version(char *content) {
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
    PkgVersion *v = malloc(sizeof(PkgVersion));
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
