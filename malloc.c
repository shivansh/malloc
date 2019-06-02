// This program implements a memory allocator using a first-fit algorithm,
// without consideration for alignment and thread safety.

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

// split_block resizes blk to size and inserts a new block of remaining size
// next to it in the free block list.
struct meta* split_block(struct meta* blk, size_t size) {
    struct meta* nblk = (void*)(blk + 1) + size;
    nblk->size = blk->size - size - META_SIZE;
    blk->size = size;
    nblk->free = 1;
    if (blk->next) {
        blk->next->prev = nblk;
    }
    nblk->prev = blk;
    nblk->next = blk->next;
    blk->next = nblk;

    // If next block is free, merge it with nblk.
    if (nblk->next && nblk->next->free) {
        nblk->size += META_SIZE + nblk->next->size;
        nblk->next = nblk->next->next;
        if (nblk->next) {
            nblk->next->prev = nblk;
        }
    }
    return nblk;
}

// find_free_block also updates tail to point to the last block in the free
// list, which is required when calling get_space. The first block of
// appropriate size is returned (after splitting, if necessary).
struct meta* find_free_block(struct meta** tail, size_t size) {
    struct meta* blk = head;
    while (blk && !(blk->free && blk->size >= size)) {
        *tail = blk;
        blk = blk->next;
    }
    // Split current block if there is enough space.
    if (blk && blk->size - size > META_SIZE) {
        struct meta* nblk = split_block(blk, size);
        if (!nblk->next) {
            *tail = nblk;
        }
    }
    return blk;
}

struct meta* get_space(struct meta* tail, size_t size) {
    struct meta* blk = sbrk(0);
    void* req = sbrk(META_SIZE + size);
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
        blk->size += META_SIZE + blk->next->size;
        blk->next = blk->next->next;
        if (blk->next) {
            blk->next->prev = blk;
        }
    }
    while (blk->prev && blk->prev->free) {
        // Merge current block into previous.
        blk->prev->size += META_SIZE + blk->size;
        blk->prev->next = blk->next;
        if (blk->next) {
            blk->next->prev = blk->prev;
        }
        blk = blk->prev;
    }
}

void* calloc(size_t nmemb, size_t elem_size) {
    size_t size = nmemb * elem_size;
    if (size <= 0) {
        return NULL;
    }
    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    struct meta* blk = get_block_addr(ptr);
    assert(blk->free == 0);
    if (blk->size >= size) {
        // Split if possible when shrinking.
        if (blk->size - size > META_SIZE) {
            split_block(blk, size);
        }
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
