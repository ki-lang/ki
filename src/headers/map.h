
#ifdef H_MAP
#else
#define H_MAP 1

#include "array.h"

typedef struct Map {
    Array *keys;
    Array *values;
} Map;

Map *map_make(Allocator *alc);
bool map_contains(Map *map, char *key);
void *map_get(Map *map, char *key);
bool map_unset(Map *map, char *key);
void map_set(Map *map, char *key, void *value);
void map_print_keys(Map *map);

#endif
