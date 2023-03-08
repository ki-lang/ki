
#ifdef H_ARR
#else
#define H_ARR 1

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
bool array_contains(Array *, void *, char *);
int array_find(Array *, void *, char *);
void *array_get_index(Array *, int);
void array_set_index(Array *, int, void *);

#endif
