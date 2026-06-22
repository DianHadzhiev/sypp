#include <stdlib.h>
#include <stdio.h>
#include "snapshot.h"
#include "stack.h"
#include "heap.h"

volatile sig_atomic_t stop_requested = 0;

void handle_sigint(int sig)
{
    (void)sig;
    stop_requested = 1;
}

void save_snapshot(ijvm* m, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open snapshot file for writing\n");
        return;
    }

    fwrite(&m->pc, sizeof(uint32_t), 1, f);
    fwrite(&m->lv, sizeof(int32_t), 1, f);
    fwrite(&m->bp, sizeof(int32_t), 1, f);
    fwrite(&m->halted, sizeof(bool), 1, f);

    fwrite(&m->stack->size, sizeof(int), 1, f);
    fwrite(m->stack->data, sizeof(int32_t), m->stack->size, f);

    fwrite(&m->constant_pool_size, sizeof(uint32_t), 1, f);
    fwrite(m->constant_pool, 1, m->constant_pool_size, f);

    fwrite(&m->text_size, sizeof(uint32_t), 1, f);
    fwrite(m->text, 1, m->text_size, f);

    fwrite(&m->heap->count, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < m->heap->count; i++) {
        Array *a = &m->heap->arrays[i];
        fwrite(&a->length, sizeof(int32_t), 1, f);
        fwrite(&a->freed, sizeof(bool), 1, f);
        if (!a->freed) {
            fwrite(a->data, sizeof(int32_t), a->length, f);
        }
    }

    fclose(f);
    fprintf(stderr, "Snapshot saved to %s\n", path);
}

ijvm* load_snapshot(const char *path, FILE *input, FILE *output)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open snapshot file %s\n", path);
        return NULL;
    }

    ijvm *m = malloc(sizeof(ijvm));
    m->in  = input;
    m->out = output;

    fread(&m->pc, sizeof(uint32_t), 1, f);
    fread(&m->lv, sizeof(int32_t), 1, f);
    fread(&m->bp, sizeof(int32_t), 1, f);
    fread(&m->halted, sizeof(bool), 1, f);

    int stack_size;
    fread(&stack_size, sizeof(int), 1, f);
    m->stack = malloc(sizeof(Stack));
    stack_init(m->stack);
    for (int i = 0; i < stack_size; i++) {
        int32_t val;
        fread(&val, sizeof(int32_t), 1, f);
        stack_push(m->stack, val);
    }

    fread(&m->constant_pool_size, sizeof(uint32_t), 1, f);
    m->constant_pool = malloc(m->constant_pool_size);
    fread(m->constant_pool, 1, m->constant_pool_size, f);

    fread(&m->text_size, sizeof(uint32_t), 1, f);
    m->text = malloc(m->text_size);
    fread(m->text, 1, m->text_size, f);

    m->heap = malloc(sizeof(Heap));
    heap_init(m->heap);
    uint32_t heap_count;
    fread(&heap_count, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < heap_count; i++) {
        int32_t length;
        bool freed;
        fread(&length, sizeof(int32_t), 1, f);
        fread(&freed, sizeof(bool), 1, f);

        int32_t *data = NULL;
        if (!freed) {
            data = malloc(length * sizeof(int32_t));
            fread(data, sizeof(int32_t), length, f);
        }
        int32_t ref = heap_register(m->heap, data, length);
        if (freed) {
            heap_free_array(m->heap, ref);
        }
    }

    fclose(f);
    return m;
}
