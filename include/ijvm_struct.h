
#ifndef IJVM_STRUCT_H
#define IJVM_STRUCT_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  /* contains type FILE * */
#include "stack.h"


/**
 * All the state of your IJVM machine goes in this struct!
 **/

typedef struct IJVM {
    // do not changes these two variables
    FILE *in;   // use fgetc(ijvm->in) to get a character from in.
                // This will return EOF if no char is available.
    FILE *out;  // use for example fprintf(ijvm->out, "%c", value); to print value to out

  // your variables go here
    uint8_t *constant_pool;
    uint32_t constant_pool_origin;       
    uint32_t constant_pool_size; 

    uint32_t text_origin;
    uint8_t *text;                
    uint32_t text_size;
    
    uint32_t pc;   
    int32_t  lv;
    int32_t bp;
    Stack *stack;

    bool halted;
} ijvm;

#endif
