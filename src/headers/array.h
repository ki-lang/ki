
#ifdef H_ARR
#else
#define H_ARR 1

enum ARRFINDTYPE {
    arr_find_adr,
    arr_find_str,
    arr_find_int,
};

typedef struct Array {
    Allocator *alc;
    int length;
    int max_length;
    void *data;
} Array;

Array *array_make(Allocator *alc, int max_length);
void array_push(Array *, void *);
void array_push_unique(Array *arr, void *item);
void array_push_unique_chars(Array *arr, void *item);
void *array_pop(Array *arr);
void *array_pop_first(Array *arr);
bool array_contains(Array *, void *, int);
int array_find(Array *, void *, int);
void array_shift(Array *arr, void *item);
void *array_get_index(Array *, int);
void array_set_index(Array *, int, void *);

#endif
