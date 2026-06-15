#ifndef STACK_H
#define STACK_H

#include <stdio.h>  /* contains type FILE * */
#include <stdint.h>  /* contains exact integer types int32_t, uint8_t */
#include <stdbool.h>


typedef struct {
    int32_t *data;
    int size;
    int capacity;
} Stack;

void     stack_init(Stack *s);
void stack_push(Stack *s, int32_t value);
int32_t stack_pop(Stack *s);
int32_t stack_top(Stack *s);
bool stack_is_empty(Stack *s);
void stack_free(Stack *s);
int32_t stack_get(Stack *s, int index);
void    stack_set(Stack *s, int index, int32_t value);

#endif
