
#include "all.h"

Array *array_make(Allocator *alc, int max_length) {
    Array *arr = al(alc, sizeof(Array));
    arr->alc = alc;
    arr->length = 0;
    arr->max_length = max_length;
    arr->data = al(alc, sizeof(void *) * max_length);
    return arr;
}

// Array *array_from_pointer_list(void *list[], int count) {
//     Array *arr = array_make(count);
//     for (int i = 0; i < count; i++) {
//         array_push(arr, list[i]);
//     }
//     return arr;
// }

void array_push(Array *arr, void *item) {
    if (arr->length == arr->max_length) {
        int newlen = arr->max_length * 2;
        void *new = al(arr->alc, newlen * sizeof(void *));
        memcpy(new, arr->data, arr->max_length * sizeof(void *));
        // free(arr->data);
        arr->data = new;
        arr->max_length = newlen;
    }
    uintptr_t *adr = arr->data + (arr->length * sizeof(void *));
    *adr = (uintptr_t)item;
    arr->length++;
}

void array_push_unique(Array *arr, void *item) {
    if (!array_contains(arr, item, arr_find_str)) {
        array_push(arr, item);
    }
}
void array_push_unique_chars(Array *arr, void *item) {
    if (!array_contains(arr, item, arr_find_str)) {
        array_push(arr, item);
    }
}

void *array_pop(Array *arr) {
    if (arr->length == 0) {
        return NULL;
    }
    arr->length--;
    uintptr_t *adr = arr->data + ((arr->length) * sizeof(void *));
    return (void *)(*adr);
}

void array_shift(Array *arr, void *item) {
    if (arr->length == arr->max_length) {
        int newlen = arr->max_length * 2;
        void *new = al(arr->alc, newlen * sizeof(void *));
        memcpy(new, arr->data, arr->max_length * sizeof(void *));
        // free(arr->data);
        arr->data = new;
        arr->max_length = newlen;
    }
    int i = arr->length;
    void *data = arr->data;
    while (i > 0) {
        void **from = data + (i - 1) * sizeof(void *);
        void **to = data + i * sizeof(void *);
        *to = *from;
        i--;
    }
    arr->length++;
    void **adr = arr->data;
    *adr = item;
}

void *array_get_index(Array *arr, int index) {
    if (index >= arr->length) {
        return NULL;
    }
    uintptr_t *result = arr->data + (index * sizeof(void *));
    return (void *)*result;
}

void array_set_index(Array *arr, int index, void *item) {
    if (index > arr->max_length - 1) {
        die("array set index out of range\n");
    }
    if ((index + 1) > arr->length) {
        arr->length = index + 1;
    }
    uintptr_t *adr = arr->data + (index * sizeof(void *));
    *adr = (uintptr_t)item;
}

bool array_contains(Array *arr, void *item, int type) {
    int index = array_find(arr, item, type);
    return index > -1;
}

int array_find(Array *arr, void *item, int type) {
    int x = arr->length;
    while (x > 0) {
        x--;
        uintptr_t *adr = arr->data + (x * sizeof(void *));
        if (type == arr_find_adr) {
            if (*adr == (uintptr_t)item)
                return x;
        } else if (type == arr_find_str) {
            char *a = (char *)*adr;
            char *b = (char *)item;
            if (strcmp(a, b) == 0)
                return x;
        } else if (type == arr_find_int) {
            if ((int)(*adr) == *(int *)item)
                return x;
        } else {
            die("array.c invalid search type");
        }
    }
    return -1;
}
