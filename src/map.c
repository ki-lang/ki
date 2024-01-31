
#include "all.h"

Map *map_make(Allocator *alc) {
    Map *m = al(alc, sizeof(Map));
    m->alc = alc;
    m->keys = array_make(alc, 4);
    m->values = array_make(alc, 4);
    m->find_start = 0;
    return m;
}

bool map_contains(Map *map, char *key) {
    // int i = array_find(map->keys, key, arr_find_str);
    // if (i == -1)
    //     return false;
    int i = array_find_x(map->keys, key, arr_find_str, map->find_start, map->keys->length);
    if (i == -1) {
        if(map->find_start > 0) {
            i = array_find_x(map->keys, key, arr_find_str, 0, map->find_start);
        }
        if(i == -1)
            return false;
    }
    map->find_start = i;
    return true;
}

void *map_get(Map *map, char *key) {
    // int i = array_find(map->keys, key, arr_find_str);
    // if (i == -1)
    //     return NULL;
    int i = array_find_x(map->keys, key, arr_find_str, map->find_start, map->keys->length);
    if (i == -1) {
        if(map->find_start > 0) {
            i = array_find_x(map->keys, key, arr_find_str, 0, map->find_start);
        }
        if(i == -1)
            return NULL;
    }
    map->find_start = i;
    return array_get_index(map->values, i);
}

void map_set(Map *map, char *key, void *value) {
    int i = array_find(map->keys, key, arr_find_str);
    if (i == -1) {
        array_push(map->keys, dups(map->alc, key));
        array_push(map->values, value);
    } else {
        array_set_index(map->values, i, value);
    }
}

bool map_unset(Map *map, char *key) {
    int i = array_find(map->keys, key, arr_find_str);
    if (i == -1) {
        return false;
    }
    array_set_index(map->values, i, NULL);
    return true;
}

void map_print_keys(Map *map) {
    int c = map->keys->length;
    printf("c:%d:", c);
    while (c > 0) {
        c--;
        char *key = array_get_index(map->keys, c);
        printf("%s,", key);
    }
    printf("\n");
}
