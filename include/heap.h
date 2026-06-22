#ifndef HEAP_H
#define HEAP_H

#include <stdio.h>  
#include <stdint.h>
#include <stdbool.h>

typedef struct{
    int32_t *data;
    int32_t length;
    bool freed;
} Array;

typedef struct {
    Array *arrays;
    uint32_t count;
    uint32_t capacity;
} Heap;

void heap_init(Heap *h);
uint32_t heap_register(Heap *h, int32_t *data, int32_t length);
void heap_free_object(Heap *h, int32_t ref);
int32_t *heap_get(Heap *h, int32_t ref);
void heap_destroy(Heap *h);
void heap_free_array(Heap *h, int32_t ref);
#endif
