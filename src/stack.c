#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

void stack_init(Stack *s) {
    s->data = malloc(256 * sizeof(int32_t));
    s->size = 0;
    s->capacity = 256;
}

void stack_push(Stack *s, int32_t value) {
   if(s->size == s->capacity){
        s->capacity *=2;
        s->data = realloc(s->data, s->capacity * sizeof(int32_t));
        if(!s->data){
            exit(1);
        }
   }
   s->data[s->size] = value;
   s->size++;
}

int32_t stack_pop(Stack *s) {
    if (s->size == 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    s->size--;
    return s->data[s->size];
}

int32_t stack_top(Stack *s) {
    if (s->size == 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    return s->data[s->size - 1];
}

bool stack_is_empty(Stack *s) {
    return s->size == 0;
}

void stack_free(Stack *s) {
    if (s == NULL) return;
    free(s->data);
    s->data = NULL;
    s->size = 0;
    s->capacity = 0;
}

int32_t stack_get(Stack *s, int index) {
    if (index < 0 || index >= s->size) {
        fprintf(stderr, "stack_get: index %d out of bounds (size %d)\n", index, s->size);
        exit(1);
    }
    return s->data[index];
}

void stack_set(Stack *s, int index, int32_t value) {
    if (index < 0 || index >= s->size) {
        fprintf(stderr, "stack_set: index %d out of bounds (size %d)\n", index, s->size);
        exit(1);
    }
    s->data[index] = value;
}
