#include <stdio.h>  // for getc, printf
#include <stdlib.h> // malloc, free
#include <signal.h>
#include "ijvm.h"
#include "util.h" // read this file for debug prints, endianness helper functions
#include "ijvm_struct.h"
#include "snapshot.h"


// see ijvm.h for descriptions of the below functions

ijvm* init_ijvm(char *binary_path, FILE* input, FILE* output)
{
  signal(SIGINT, handle_sigint);

  ijvm* m = (ijvm *) malloc(sizeof(ijvm));

  m->in = input;
  m->out = output;
  
  m->pc = 0;
  m->lv = 0;
  m->bp = -1;
  m->constant_pool = NULL;
  m->text = NULL;
  m->stack = NULL;
  m->constant_pool_size = 0;
  m->text_size = 0;
  m->halted = false;
  m->heap = NULL;

  // read binary file, rb
  FILE* f = fopen(binary_path, "rb");
  
  uint8_t buf[4];
  
  //read first 4 bytes, magic number
  fread(buf, 4, 1, f);
  if (read_uint32(buf) != 0x1DEADFAD){
    fclose(f);
    destroy_ijvm(m);
    return NULL;
  }

  //read constant origin
  fread(buf, 4, 1, f);
  m->constant_pool_origin = read_uint32(buf);
  //read constant size
  fread(buf, 4, 1, f);
  m->constant_pool_size = read_uint32(buf);

  m->constant_pool = malloc(m->constant_pool_size);

  if (m->constant_pool == NULL) { 
    fclose(f); 
    destroy_ijvm(m);
    return NULL;
  }
  // read constant pool
  fread(m->constant_pool, 1, m->constant_pool_size, f);

  // Read text section
  fread(buf, 4, 1, f);
  m->text_origin = read_uint32(buf);
  fread(buf, 4, 1, f); 
  m->text_size  = read_uint32(buf);

  m->text = malloc(m->text_size); 
  if (m->text == NULL) { 
    fclose(f); 
    destroy_ijvm(m);
    return NULL; 
  }
  
  //read text
  fread(m->text, 1, m->text_size, f);

  // Allocate stack memory with size of 256 32 bit integers
  m->stack = malloc(sizeof(Stack));
  stack_init(m->stack);
  for (int i = 0; i < 256; i++) {
      stack_push(m->stack, 0);
  }

  m->heap = malloc(sizeof(Heap));
  if (m->heap == NULL) {
      fclose(f);
      destroy_ijvm(m);
      return NULL;
  }
  heap_init(m->heap);

  m->lv = 0;

  fclose(f);
  return m;
}

void destroy_ijvm(ijvm* m) 
{
  if (m == NULL) return;
  free(m->constant_pool);
  free(m->text);
  if (m->stack != NULL) {
      stack_free(m->stack);
      free(m->stack);
  }
  if (m->heap != NULL) {
        heap_destroy(m->heap);
        free(m->heap);
    }
  free(m); // free memory for struct
}

uint8_t *get_text(ijvm* m) 
{
  return m->text;
}

uint32_t get_text_size(ijvm* m) 
{
    return m->text_size;
}

int32_t get_constant(ijvm* m, uint32_t i) 
{
  uint32_t byte_offset = i * 4; 
  return read_int32(&m->constant_pool[byte_offset]);
}

uint32_t get_program_counter(ijvm* m) 
{
  return m->pc;
}

int32_t tos(ijvm* m) 
{
  return stack_top(m->stack);
}

// returns true if program is halted or pc has reached end of text section i.e.
// all instructions stored in text section are executed
bool finished(ijvm* m) 
{
  return m->halted || m->pc >= m->text_size;
}

// return local variable of a function at index i inside stack frame
int32_t get_local_variable(ijvm* m, uint32_t i) 
{
  return stack_get(m->stack, m->lv + i);
}

int16_t calc_offset(ijvm* m){
  uint8_t byte1 = m->text[m->pc]; // shift first byte 8 bits to the left
  uint8_t byte2 = m->text[m->pc + 1]; // add second byte 
  int16_t offset = (int16_t)((byte1 << 8) | byte2); // OR operations to append byte 2
  return offset;
}

void step(ijvm* m) 
{
  uint8_t instruction = get_instruction(m);
  m->pc++;

  switch(instruction)
  {
    case OP_BIPUSH:
    {
      int8_t operand = (int8_t)m->text[m->pc];
      m->pc++;
      stack_push(m->stack, operand);
      break;
    }

    case OP_IAND:
    {
      int32_t top = stack_pop(m->stack);
      int32_t val2 = stack_pop(m->stack);
      int32_t res = top & val2;
      stack_push(m->stack, res);
      break;
    }
    
    case OP_DUP:
    {
      int32_t val = stack_top(m->stack);
      stack_push(m->stack, val);
      break;
    }

    case OP_IADD:
    {
      int32_t val1 = stack_pop(m->stack);
      int32_t val2 = stack_pop(m->stack);
      int32_t res = val1 + val2;
      stack_push(m->stack, res);
      break;
    }

    case OP_IOR:
    {
      int32_t val1 = stack_pop(m->stack);
      int32_t val2 = stack_pop(m->stack);
      int32_t res = val1 | val2;
      stack_push(m->stack, res);
      break;
    }
    
    case OP_ISUB:
    {
      int32_t val1 = stack_pop(m->stack);
      int32_t val2 = stack_pop(m->stack);
      int32_t res = val2 - val1;
      stack_push(m->stack, res);
      break;
    }
    
    case OP_NOP:

      break;
    
    case OP_SWAP:
    {
      int32_t top = stack_pop(m->stack);
      int32_t newTop = stack_pop(m->stack);
      stack_push(m->stack, top);
      stack_push(m->stack, newTop);
      break;
    }
    
    case OP_ERR:
      fprintf(m->out, "Error!\n");
      m->halted = true;
      break;

    case OP_HALT:
      m->halted = true;
      break;

    case OP_IN:
    {
      int value = fgetc(m->in);
      if (value == EOF) value = 0;
      stack_push(m->stack, value);
      break;
    }
    
    case OP_OUT:
    {
      int value = stack_pop(m->stack);
      fprintf(m->out, "%c", value);
      break;
    }

    case OP_POP:
      stack_pop(m->stack);
      break;

    case OP_GOTO:
    {
      int offset = calc_offset(m);
      uint32_t opcode_address = m->pc - 1;
      m->pc= opcode_address + offset;
      break;
    }

    case OP_IFEQ:
    {
      int32_t word = stack_pop(m->stack);
      int offset = calc_offset(m);

      uint32_t opcode_address = m->pc - 1;

      if (word == 0) {
        m->pc = opcode_address + offset;
      } 
      else {
        m->pc += 2; // skip 2 bytes i.e. short argument
      }
      break;
    }

    case OP_IFLT:
    {
      int32_t word = stack_pop(m->stack);
      int offset = calc_offset(m);
      uint32_t opcode_address = m->pc - 1;

      if (word < 0) {
        m->pc = opcode_address + offset;
      } 
      else {
        m->pc += 2;
      }
      break;
    }

    case OP_IF_ICMPEQ:
    {
      int32_t word1 = stack_pop(m->stack);
      int32_t word2 = stack_pop(m->stack);
      int offset = calc_offset(m);

      uint32_t opcode_address = m->pc - 1;

      if (word1 == word2) {
        m->pc = opcode_address + offset;
      } 
      else {
        m->pc += 2;
      }
      break;
    }

    case OP_LDC_W:
    {
      uint16_t index =(uint16_t) ((m->text[m->pc] << 8) | m->text[m->pc + 1]);
      m->pc+= 2;
      int32_t constants = get_constant(m, index);
      stack_push(m->stack, constants);
      break;
    }

    case OP_IINC:
    {
      uint8_t i = m->text[m->pc]; 
      int8_t val = m->text[m->pc + 1]; // value to increment variable at index i

      int32_t initialVal = stack_get(m->stack, m->lv + i);
      stack_set(m->stack, m->lv + i, initialVal + val);

      m->pc+=2;
      break;

    }

    case OP_ILOAD:
    {
      uint8_t i = m->text[m->pc];
      m->pc += 1;
      int32_t var = get_local_variable(m, i);

      stack_push(m->stack, var);
      break;
    }

    case OP_ISTORE:
    {
      int32_t word = stack_pop(m->stack);
      uint8_t i = (uint8_t)m->text[m->pc];
      m->pc += 1;
      stack_set(m->stack, m->lv + i, word);
      break;
    }

    case OP_WIDE:
    {
      uint8_t op = m->text[m->pc];
      m->pc++;

      switch (op)
      {
        case OP_IINC:
        {
        uint16_t i  = (uint16_t) (m->text[m->pc] << 8) | m->text[m->pc + 1];
        int8_t val  = (int8_t)m->text[m->pc + 2];  
        int32_t cur = stack_get(m->stack, m->lv + i);
        stack_set(m->stack, m->lv + i, cur + val);
        m->pc += 3;   
        break;
        }
        
        case OP_ILOAD:
        {
          uint16_t i = (m->text[m->pc] << 8) | m->text[m->pc + 1]; 
          m->pc += 2;
          stack_push(m->stack, stack_get(m->stack, m->lv + i));
          break;
        }

        case OP_ISTORE:
        {
          uint16_t i = (uint16_t)((m->text[m->pc] << 8) | m->text[m->pc + 1]);
          m->pc += 2;
          stack_set(m->stack, m->lv + i, stack_pop(m->stack));
          break;
        }

        default:
          m->halted = true;
          break;
      }
      break;
    }

    case OP_INVOKEVIRTUAL:
    {
      uint16_t index  = (m->text[m->pc] << 8) | m->text[m->pc + 1];
      m->pc += 2;

      int32_t methodAreaIndex = get_constant(m, index);
      uint16_t numArgs = (m->text[methodAreaIndex] << 8) | (m->text[methodAreaIndex + 1]);
      uint16_t numLocals = (m->text[methodAreaIndex + 2] << 8) | (m->text[methodAreaIndex + 3]);
      
      //save frame state for return
      int32_t old_lv = m->lv;
      int32_t old_sp = m->stack->size - 1 - numArgs; // args arleady pushed at this point
      int32_t old_pc = m->pc;
      int32_t old_bp = m->bp; 

      for(int i = 0; i < numLocals; i++){
        stack_push(m->stack, 0);
      }

      int32_t new_bp = m->stack->size; // start of frame info

      stack_push(m->stack, old_pc); 
      stack_push(m->stack, old_bp);
      stack_push(m->stack, old_lv);
      stack_push(m->stack, old_sp);
      
      m->lv = new_bp - numLocals - numArgs; // index of OBJREF
      m->bp = new_bp;
      m->pc = methodAreaIndex + 4;

      break;
    }
    
    case OP_IRETURN:
    {
      int32_t ret_value = stack_pop(m->stack); // value returned by method
      m->stack->size = m->bp + 4;

      //pop old state in same order
      int32_t old_sp = stack_pop(m->stack);
      int32_t old_lv = stack_pop(m->stack);
      int32_t old_bp = stack_pop(m->stack);
      int32_t old_pc = stack_pop(m->stack);

      // restore state
      m->stack->size = old_sp + 1;
      m->lv = old_lv;
      m->pc = old_pc;
      m->bp = old_bp;

      stack_push(m->stack, ret_value); // push returned value for further computation

      break;
    }

    case OP_TAILCALL:
    {
      uint16_t index = (m->text[m->pc] << 8) | m->text[m->pc + 1];
      m->pc += 2;

      int32_t methodAreaIndex = get_constant(m, index);
      uint16_t numArgs   = (m->text[methodAreaIndex]     << 8) | m->text[methodAreaIndex + 1];
      uint16_t numLocals = (m->text[methodAreaIndex + 2] << 8) | m->text[methodAreaIndex + 3];

      int32_t new_args_start = m->stack->size - numArgs;

      int32_t old_pc = stack_get(m->stack, m->bp);
      int32_t old_bp = stack_get(m->stack, m->bp + 1);
      int32_t old_lv = stack_get(m->stack, m->bp + 2);
      int32_t old_sp = stack_get(m->stack, m->bp + 3);
      
      //overwrite current frame with new args
      for(int i = 0; i < numArgs; i++){
        int32_t val = stack_get(m->stack, new_args_start + i);
        stack_set(m->stack, m->lv + i, val);
      }

      //remove current frame
      m->stack->size = m->lv + numArgs;

      for(int i = 0; i < numLocals; i++) {
        stack_push(m->stack, 0);
      }

      m->bp = m->stack->size;

      stack_push(m->stack, old_pc);
      stack_push(m->stack, old_bp);
      stack_push(m->stack, old_lv);
      stack_push(m->stack, old_sp);

      m->pc = methodAreaIndex + 4;
      break;
    }

    case OP_NEWARRAY:
    {
      int32_t count = stack_pop(m->stack);

      int32_t *data = calloc(count, sizeof(int32_t));

      if(data == NULL){
        fprintf(stderr, "Out of memory\n");
        exit(1);
      }

      for(int i = 0; i < count; ++i){
        data[i] = 0;
      }

      int32_t ref = heap_register(m->heap, data, count);
      stack_push(m->stack, ref);
      break;
    }

    case OP_IALOAD:
    {
      int32_t arrayref = stack_pop(m->stack);
      int32_t index = stack_pop(m->stack);

      int32_t *array = heap_get(m->heap, arrayref);  
      int32_t value = array[index];                 

      stack_push(m->stack, value);
      break;

    }

    case OP_IASTORE:
    {
      int32_t arrayref = stack_pop(m->stack);
      int32_t index = stack_pop(m->stack);
      int32_t value = stack_pop(m->stack);

      int32_t *array = heap_get(m->heap, arrayref);
      array[index] = value;

      break;
    }

    default:
      m->halted = true;
      break;
  }

}

uint8_t get_instruction(ijvm* m) 
{ 
  return get_text(m)[get_program_counter(m)]; 
}

ijvm* init_ijvm_std(char *binary_path) 
{
  return init_ijvm(binary_path, stdin, stdout);
  
}

void run(ijvm* m) 
{
  while (!finished(m)) 
  {
    step(m);
    if(stop_requested){
      save_snapshot(m, "snapshot.ijvmstate");
      m->halted = true;
      break;
    }
    
  }
}

// Below: methods needed by bonus assignments, see ijvm.h
// You can leave these unimplemented if you are not doing these bonus 
// assignments.

uint32_t get_call_stack_size(ijvm* m) 
{
   uint32_t count = 0;
   int32_t bp = m->bp;

   // loop continues unitl it walks past first bp
   while (bp != -1){
    count ++;
    bp = stack_get(m->stack, bp + 1); // get old bp
   }

   return count;
}

// Checks if reference is a freed heap array. Note that this assumes that 
// 
bool is_heap_freed(ijvm* m, uint32_t reference) 
{
   return m->heap->arrays[reference].freed;
}

// Checks if top of stack is a reference
bool is_tos_reference(ijvm* m)
{
	// TODO: implement me if doing precise garbage collection bonus
	//  using ANEWARRAY, AIALOAD and AIASTORE
	return false;
}


