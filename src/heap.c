#include <stdio.h>
#include <stdlib.h>
#include "heap.h"
#include "util.h"

void heap_init(Heap *h)
{
    h->capacity = 32;
    h->count = 0;
    h->arrays = malloc(32 * sizeof(Array));
    if (h->arrays == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
}

uint32_t heap_register(Heap *h, int32_t *data, int32_t length)
{
    if (h->count == h->capacity) {
        h->capacity *= 2;
        h->arrays = realloc(h->arrays, h->capacity * sizeof(Array));
        if (h->arrays == NULL) {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
    }

    uint32_t ref = h->count;
    h->arrays[ref].data = data;
    h->arrays[ref].length = length;
    h->arrays[ref].freed = false;
    h->count++;
    return ref;
}

void heap_free_array(Heap *h, int32_t ref) 
{
    free(h->arrays[ref].data);
    h->arrays[ref].data = NULL;
    h->arrays[ref].freed = true;

}

int32_t *heap_get(Heap *h, int32_t ref)
{
    return h->arrays[ref].data;
}

void heap_destroy(Heap *h)
{
    for (uint32_t i = 0; i < h->count; i++) {
        if(!h->arrays[i].freed){
            free(h->arrays[i].data);
        }
    }
    free(h->arrays);
}
