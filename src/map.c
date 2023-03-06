
#include "all.h"

Map *map_make() {
    Map *m = malloc(sizeof(Map));
    m->keys = array_make(4);
    m->values = array_make(4);
    return m;
}

bool map_contains(Map *map, char *key) {
    int i = array_find(map->keys, key, "chars");
    if (i == -1) {
        return false;
    }
    return true;
}

void *map_get(Map *map, char *key) {
    int i = array_find(map->keys, key, "chars");
    if (i == -1) {
        return NULL;
    }
    return array_get_index(map->values, i);
}

void map_set(Map *map, char *key, void *value) {
    int i = array_find(map->keys, key, "chars");
    if (i == -1) {
        array_push(map->keys, strdup(key));
        array_push(map->values, value);
    } else {
        array_set_index(map->values, i, value);
    }
}

bool map_unset(Map *map, char *key) {
    int i = array_find(map->keys, key, "chars");
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

void map_free(Map *map, bool free_values) {
    array_free(map->keys, false);
    if (free_values) {
        array_free(map->values, false);
    }
    free(map);
}
