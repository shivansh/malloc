#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct meta {
    size_t size;
    int free;
    struct meta* next;
    struct meta* prev;
};

#define META_SIZE sizeof(struct meta)

struct meta* head = NULL;

// find_free_block also updates tail to point to the last block in the free
// list, which is required when calling get_space.
struct meta* find_free_block(struct meta** tail, size_t size) {
    struct meta* blk = head;
    while (blk && !(blk->free && blk->size >= size)) {
        *tail = blk;
        blk = blk->next;
    }
    return blk;
}

struct meta* get_space(struct meta* tail, size_t size) {
    struct meta* blk = sbrk(0);
    void* req = sbrk(size + META_SIZE);
    assert((void*)blk == req);
    if (req == (void*)-1) {
        return NULL;
    }
    if (tail) {
        tail->next = blk;
    }
    blk->prev = tail;
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
    if (!head) {  // first call
        blk = get_space(NULL, size);
        if (!blk) {
            return NULL;
        }
        head = blk;
    } else {
        struct meta* tail = head;
        blk = find_free_block(&tail, size);
        if (!blk) {
            blk = get_space(tail, size);
        } else {
            blk->free = 0;  // found a free block
        }
    }
    return blk + 1;
}

void free(void* ptr) {
    if (!ptr) {
        return;
    }
    struct meta* blk = get_block_addr(ptr);
    assert(blk->free == 0);
    blk->free = 1;
    // Check for adjacent free blocks.
    while (blk->next && blk->next->free) {
        // Merge next block into current.
        blk->size += blk->next->size + META_SIZE;
        blk->next = blk->next->next;
    }
    while (blk->prev && blk->prev->free) {
        // Merge current block into previous.
        blk->prev->size += blk->size + META_SIZE;
        blk->prev->next = blk->next;
        blk = blk->prev;
    }
}

void* calloc(size_t nmemb, size_t elem_size) {
    size_t size = nmemb * elem_size;
    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    struct meta* blk = get_block_addr(ptr);
    if (blk->size >= size) {
        return ptr;
    }
    void* new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    memcpy(new_ptr, ptr, blk->size);
    free(ptr);
    return new_ptr;
}
