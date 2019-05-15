#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct meta {
    size_t size;
    struct meta* next;
    int free;
};

struct meta* head = NULL;

struct meta* find_free_block(size_t size) {
    struct meta* blk = head;
    while (blk) {
        if (blk->size >= size) {
            return blk;
        } else {
            blk = blk->next;
        }
    }
    return blk;
}

struct meta* get_space(struct meta* tail, size_t size) {
    struct meta* blk = sbrk(0);
    struct meta* req = sbrk(sizeof(struct meta) + size);
    assert(blk == req);
    if (req == (void*)-1) {
        return NULL;
    }
    if (tail) {
        tail->next = blk;
    }
    blk->size = size;
    blk->next = NULL;
    blk->free = 0;
    return blk;
}

struct meta* get_block_addr(void* ptr) {
    return (struct meta*)ptr - 1;
}

void* malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }
    struct meta* blk;
    if (!head) {
        // first call
        blk = get_space(NULL, size);
        head = blk;
    } else {
        blk = find_free_block(size);
        if (!blk) {
            struct meta* tail = get_block_addr(sbrk(0));
            blk = get_space(tail, size);
        }
    }
    if (!blk) {
        return NULL;
    } else {
        return blk + 1;
    }
}

void free(void* ptr) {
    if (!ptr) {
        return;
    }
    struct meta* blk = get_block_addr(ptr);
    assert(blk->free == 0);
    blk->free = 1;
}

void* calloc(size_t nmemb, size_t elem_size) {
    size_t size = nmemb * elem_size;
    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    struct meta* blk = get_block_addr(ptr);
    if (blk->size >= size) {
        return ptr;
    } else {
        void* new_ptr = malloc(size);
        memcpy(new_ptr, ptr, blk->size);
        free(ptr);
        return new_ptr;
    }
}
