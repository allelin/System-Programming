/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "errno.h"
/* Basic constants and macros */
#define WSIZE 8             /* Word and header/footer size (bytes) */
#define DSIZE 16            /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */
#define MAX(x, y) ((x) > (y) ? (x) : (y))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
/* Read and write a word at address p */
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define IN_QUICK(p) (GET(p) & 0x4)
#define GET_PRE_ALLOC(p) (GET(p) & 0x2)
#define GET_PREV_QUICK(p) (GET(p) & 0x4)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE((bp)) - WSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// Make function for index of free list
int GET_INDEX_FREE(size_t size, size_t M) {
    // int max_index = NUM_FREE_LISTS - 1;
    int max = 2 << NUM_FREE_LISTS;
    int max_size = max * M;
    if (size > max_size) {
        return -1;
    } else {
        int index = 0;
        while (size > M) {
            M = M * 2;
            index++;
        }
        return index;
    }
}

// Make function for index of quick list
int GET_INDEX_QUICK(size_t size, size_t M) {
    int max = M + (NUM_QUICK_LISTS - 1) * 8;
    if (size > max) {
        return -1;
    }
    size_t s = (size - M) / 8;
    if (s < 0) {
        return -1;
    }
    return s;
}

void coalesce(sf_block *block) {
    // sf_block *block = (sf_block *)((char *)pp - 8);
    size_t size = GET_SIZE((block));
    size_t new_size = size;
    PUT(block, size | GET_PRE_ALLOC(block));

    // define the header and footer
    sf_block *header = block;
    // sf_block *footer = (sf_block *)((char *)block + size - 8);

    // insert the block into the free list
    // checking the next block
    sf_block *next_header = (sf_block *)((char *)block + size);
    // PUT(next_header, GET_SIZE(next_header) | GET_ALLOC(next_header) | GET_PRE_ALLOC(next_header) | GET_PREV_QUICK(next_header));
    if (next_header < (sf_block *)sf_mem_end() && GET_ALLOC(next_header) == 0) {
        // first we need to free the next block from the free list
        next_header->body.links.next->body.links.prev = next_header->body.links.prev;
        next_header->body.links.prev->body.links.next = next_header->body.links.next;

        // then we need to coalesce the two blocks
        size_t next_size = GET_SIZE(next_header);
        size_t current_size = GET_SIZE(block);
        new_size = next_size + current_size;
        sf_block *next_footer = (sf_block *)((char *)next_header + next_size - 8);
        sf_block *header = block;
        sf_block *footer = next_footer;
        PUT(header, new_size | GET_PRE_ALLOC(block));
        PUT(footer, new_size | GET_PRE_ALLOC(block));
        // set the next next block's prev_alloc bit
        sf_block *next_next_header = (sf_block *)((char *)next_header + next_size);
        PUT(next_next_header, GET_SIZE(next_next_header) | GET_ALLOC(next_next_header) | GET_PREV_QUICK(next_next_header));
        PUT(FTRP(next_next_header), GET_SIZE(next_next_header) | GET_ALLOC(next_next_header) | GET_PREV_QUICK(next_next_header));
    }

    // checking the prev block
    if (GET_PRE_ALLOC(block) == 0) {
        size_t prev_size = GET_SIZE(((sf_block *)((char *)block - 8)));
        // first we need to free the previous block from the free list
        sf_block *prev_header = (sf_block *)((char *)block - prev_size);
        prev_header->body.links.next->body.links.prev = prev_header->body.links.prev;
        prev_header->body.links.prev->body.links.next = prev_header->body.links.next;

        // then we need to coalesce the two blocks
        size_t current_size = GET_SIZE(block);
        size_t new_size = prev_size + current_size;
        sf_block *header = (sf_block *)((char *)block - prev_size);
        sf_block *footer = (sf_block *)((char *)block - 8);
        PUT(header, new_size | GET_PRE_ALLOC(((sf_block *)((char *)block))));
        PUT(footer, new_size | GET_PRE_ALLOC(((sf_block *)((char *)block))));
        PUT(next_header, GET_SIZE(next_header) | GET_ALLOC(next_header) | GET_PREV_QUICK(next_header));
        PUT(FTRP(next_header), GET_SIZE(next_header) | GET_ALLOC(next_header) | GET_PREV_QUICK(next_header));
        // then we need to update the block to the new block
        // sf_block *next = (sf_block *)((char *)block + size);
        // PUT(next, GET_SIZE(next) | GET_ALLOC(next) | GET_PREV_QUICK(next));
        // PUT(FTRP(next), GET_SIZE(next) | GET_ALLOC(next) | GET_PREV_QUICK(next));
    }
    // if its neither the next or previous block is free we just need to add the footer
    if (GET_PRE_ALLOC(block) != 0 && GET_ALLOC(next_header) != 0) {
        sf_block *footer = (sf_block *)((char *)block + size - 8);
        PUT(footer, size | GET_PRE_ALLOC(block));
        PUT(header, size | GET_PRE_ALLOC(block));

        // set the next block's prev_alloc bit
        sf_block *next = (sf_block *)((char *)block + size);
        PUT(next, GET_SIZE(next) | GET_ALLOC(next) | GET_PREV_QUICK(next));
        PUT(FTRP(next), GET_SIZE(next) | GET_ALLOC(next) | GET_PREV_QUICK(next));
    }
    // add the block to the free list
    int index = GET_INDEX_FREE(new_size, 32);
    if (index != -1) {
        sf_free_list_heads[index].body.links.next->body.links.prev = header;
        header->body.links.next = sf_free_list_heads[index].body.links.next;
        header->body.links.prev = &sf_free_list_heads[index];
        sf_free_list_heads[index].body.links.next = header;
    }
}

void *sf_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    size_t min = 32;
    // add 8 to the size to account for the header
    sf_block *found_free_block = NULL;
    size += 8;
    // check if the size is 8 alligned
    if (size < min) {
        size = min;
    }
    if (size % 8 != 0) {
        // if not allign it
        size = size + (8 - (size % 8));
    }
    if (sf_mem_start() == sf_mem_end()) {
        if (sf_mem_grow() == NULL) {
            sf_errno = ENOMEM;
            return NULL;
        }
        ((sf_block *)(sf_mem_start()))->header = 32 | THIS_BLOCK_ALLOCATED;
        ((sf_block *)(sf_mem_end() - 8))->header |= THIS_BLOCK_ALLOCATED;
        ((sf_block *)(sf_mem_start() + 32))->header = (4096 - 32 - 8) | PREV_BLOCK_ALLOCATED;
        ((sf_block *)(sf_mem_start() + (4096 - 16)))->header = (4096 - 32 - 8) | PREV_BLOCK_ALLOCATED;
        size_t s = 32;
        size_t size_of_free = GET_SIZE(((sf_block *)(sf_mem_start() + 32)));
        for (int i = 0; i < NUM_FREE_LISTS; i++) {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }
        for (int i = 0; i < NUM_FREE_LISTS; i++) {
            if (i == 0) {
                if (s == size_of_free) {
                    sf_free_list_heads[i].body.links.next = (sf_block *)(sf_mem_start() + s);
                    sf_free_list_heads[i].body.links.prev = (sf_block *)(sf_mem_start() + s);
                    ((sf_block *)(sf_mem_start() + s))->body.links.next = &sf_free_list_heads[i];
                    ((sf_block *)(sf_mem_start() + s))->body.links.prev = &sf_free_list_heads[i];
                }
            } else if ((size_of_free > s) && (size_of_free <= (s * 2))) {
                sf_free_list_heads[i].body.links.next = (sf_block *)(sf_mem_start() + 32);
                sf_free_list_heads[i].body.links.prev = (sf_block *)(sf_mem_start() + 32);
                ((sf_block *)(sf_mem_start() + 32))->body.links.next = &sf_free_list_heads[i];
                ((sf_block *)(sf_mem_start() + 32))->body.links.prev = &sf_free_list_heads[i];
            } else if (i == NUM_FREE_LISTS - 1 && size_of_free > (s << 1)) {
                sf_free_list_heads[i].body.links.next = (sf_block *)(sf_mem_start() + 32);
                sf_free_list_heads[i].body.links.prev = (sf_block *)(sf_mem_start() + 32);
                ((sf_block *)(sf_mem_start() + 32))->body.links.next = &sf_free_list_heads[i];
                ((sf_block *)(sf_mem_start() + 32))->body.links.prev = &sf_free_list_heads[i];
            }
            if (i != 0) {
                s = s * 2;
            }
        }
        // setting the free list nodes to itself
    }
    // getting the index needed for the quick list
    int index = GET_INDEX_QUICK(size, 32);
    if (index != -1) {
        if (index < NUM_QUICK_LISTS) {
            if (sf_quick_lists[index].length != 0) {
                // we get the first block in the quick list
                sf_block *block = sf_quick_lists[index].first;
                // we remove the block from the quick list
                sf_quick_lists[index].first = sf_quick_lists[index].first->body.links.next;
                sf_quick_lists[index].length--;
                // we set the header
                block->header = size | THIS_BLOCK_ALLOCATED | GET_PRE_ALLOC(block);
                // set the header for the next block
                // we return the pointer to the block
                return (void *)(block->body.payload);
                // found_free_block = block;
            }
        }
    }

    // we go to the free list
    if (found_free_block == NULL) {
        int index_free = GET_INDEX_FREE(size, 32);
        if (index_free != -1) {
            for (int i = index_free; i < NUM_FREE_LISTS; i++) {
                sf_block *block = sf_free_list_heads[i].body.links.next;
                // check if the block is null
                if (block == NULL) {
                    sf_errno = ENOMEM;
                    return NULL;
                }
                while (block != &sf_free_list_heads[i]) {
                    if (GET_SIZE(block) >= size) {
                        found_free_block = block;
                        // we remove the block from the free list
                        // block->body.links.prev->body.links.next = block->body.links.next;
                        // block->body.links.next->body.links.prev = block->body.links.prev;
                        break;
                    }
                    block = block->body.links.next;
                }
                if (found_free_block != NULL) {
                    break;
                }
            }
        }
    }
    // if null we need to make a new page by calling grow
    if (found_free_block == NULL) {
        // we call grow
        sf_block *new_page = sf_mem_grow();
        // we check if the new page is null
        if (new_page == NULL) {
            sf_errno = ENOMEM;
            return NULL;
        }
        sf_block *mem_end = sf_mem_end();

        // we get the new page
        // we set the header and footer for the new page
        sf_block *end_new_page = (sf_block *)((char *)mem_end - 8);
        sf_block *start_new_page = (sf_block *)((char *)new_page - 8);
        sf_block *footer_new_page = (sf_block *)((char *)mem_end - 16);
        start_new_page->header = (PAGE_SZ) | GET_PRE_ALLOC(start_new_page);
        footer_new_page->header = (PAGE_SZ) | GET_PRE_ALLOC(start_new_page);
        end_new_page->header = THIS_BLOCK_ALLOCATED;  // This is the epilogue
        // checking if old page has space
        sf_block *old_page = (sf_block *)((char *)new_page - 8);
        // we check if the old page is free if it is we need to coalesce
        if (GET_PRE_ALLOC(old_page) == 0) {
            size_t old_page_size = GET_SIZE((char *)old_page - 8);
            // we remove the old page from the free list
            sf_block *start_old_page = (sf_block *)((char *)old_page - old_page_size);
            found_free_block = start_old_page;

            start_old_page->body.links.next->body.links.prev = start_old_page->body.links.prev;
            start_old_page->body.links.prev->body.links.next = start_old_page->body.links.next;
            // we set the header and footer;B
            size_t total = old_page_size + PAGE_SZ;
            start_old_page->header = total | GET_PRE_ALLOC(start_old_page);
            PUT(FTRP(start_old_page), total | GET_PRE_ALLOC(start_old_page));
            // if the total isn't enough we need to add more pages
            while (size > total) {
                // we call grow
                sf_block *new_page = sf_mem_grow();
                // we check if the new page is null
                if (new_page == NULL) {
                    // start_new_page->header = total | GET_PRE_ALLOC(start_new_page);
                    // PUT(FTRP(start_new_page), total | GET_PRE_ALLOC(start_new_page));
                    // set the header and footer for the block

                    // add the start old page back to the free list using get_size(start_old_page)
                    size_t s = 32;
                    size_t size_of_free = GET_SIZE((char *)start_old_page);
                    for (int i = 0; i < NUM_FREE_LISTS; i++) {
                        if (i == 0) {
                            if (s == size_of_free) {
                                sf_free_list_heads[i].body.links.next = start_old_page;
                                sf_free_list_heads[i].body.links.prev = start_old_page;
                                (start_old_page)->body.links.next = &sf_free_list_heads[i];
                                (start_old_page)->body.links.prev = &sf_free_list_heads[i];
                            }
                        } else if ((size_of_free > s) && (size_of_free <= (s * 2))) {
                            sf_free_list_heads[i].body.links.next = start_old_page;
                            sf_free_list_heads[i].body.links.prev = start_old_page;
                            start_old_page->body.links.next = &sf_free_list_heads[i];
                            start_old_page->body.links.prev = &sf_free_list_heads[i];
                        } else if (i == NUM_FREE_LISTS - 1 && size_of_free > (s << 1)) {
                            sf_free_list_heads[i].body.links.next = start_old_page;
                            sf_free_list_heads[i].body.links.prev = start_old_page;
                            start_old_page->body.links.next = &sf_free_list_heads[i];
                            start_old_page->body.links.prev = &sf_free_list_heads[i];
                        }
                        if (i != 0) {
                            s = s * 2;
                        }
                    }
                    // setting up epilogue
                    sf_block *epilogue = (sf_block *)((char *)sf_mem_end() - 8);
                    epilogue->header = 0 | THIS_BLOCK_ALLOCATED;
                    sf_errno = ENOMEM;
                    return NULL;
                }
                // we get the new page
                // we set the header and footer for the new page
                sf_block *mem_end = sf_mem_end();
                sf_block *end_new_page = (sf_block *)((char *)mem_end - 8);
                sf_block *start_new_page = (sf_block *)((char *)new_page - 8);
                start_new_page->header = (PAGE_SZ) | GET_PRE_ALLOC(start_new_page);
                end_new_page->header = (PAGE_SZ) | GET_PRE_ALLOC(end_new_page);
                if (GET_PRE_ALLOC(start_new_page) == 0) {
                    total += GET_SIZE((char *)start_new_page);
                    start_old_page->header = total | GET_PRE_ALLOC(start_old_page);
                    PUT(FTRP(start_old_page), total | GET_PRE_ALLOC(start_old_page));
                    // debug("%p", FTRP(start_old_page));
                    // debug("%p", (start_old_page));
                    // debug("%zu", (size_t)FTRP(start_old_page) - (size_t)start_old_page);
                }
            }
            // setting up epilogue
            sf_block *epilogue = (sf_block *)((char *)sf_mem_end() - 8);
            epilogue->header = 0 | THIS_BLOCK_ALLOCATED;
        }
        // if the old page is allocated we just need to set the header and footer
        if (GET_PRE_ALLOC(old_page) != 0) {
            // we set the header and footer;
            size_t total = PAGE_SZ;
            sf_block *b = (sf_block *)((char *)new_page - 8);
            b->header = total | GET_PRE_ALLOC(b);
            found_free_block = b;
            PUT(FTRP(b), total | GET_PRE_ALLOC(b));
            // if the total isn't enough we need to add more pages
            while (size > total) {
                // we call grow
                sf_block *new_page = sf_mem_grow();
                // we check if the new page is null
                if (new_page == NULL) {
                    sf_errno = ENOMEM;
                    return NULL;
                }
                // we get the new page
                // we set the header and footer for the new page
                sf_block *mem_end = sf_mem_end();
                sf_block *end_new_page = (sf_block *)((char *)mem_end - 8);
                sf_block *start_new_page = (sf_block *)((char *)new_page - 8);
                start_new_page->header = (PAGE_SZ) | GET_PRE_ALLOC(start_new_page);
                end_new_page->header = (PAGE_SZ) | GET_PRE_ALLOC(end_new_page);
                // we set the header and footer for the old page
                sf_block *old_page = (sf_block *)((char *)new_page - 16);
                size_t old_page_size = GET_SIZE(old_page);
                b = (sf_block *)((char *)old_page - old_page_size);
                // start_new_page->header = (old_page_size + PAGE_SZ - 8) | GET_PRE_ALLOC(start_new_page);
                // PUT(FTRP(start_new_page), (old_page_size + PAGE_SZ - 8) | GET_PRE_ALLOC(start_new_page));
                total += PAGE_SZ;
                // update the header and footer with the total
                b->header = total | GET_PRE_ALLOC(b);
                PUT(FTRP(b), total | GET_PRE_ALLOC(b));
            }
            coalesce(b);
        }
    }
    // after finding the free block we need to split it
    if (found_free_block != NULL) {
        // we remove the block from the free list

        found_free_block->body.links.next->body.links.prev = found_free_block->body.links.prev;
        found_free_block->body.links.prev->body.links.next = found_free_block->body.links.next;
        size_t holder = GET_SIZE(found_free_block);
        // we set the header and footer
        // found_free_block->header = size | THIS_BLOCK_ALLOCATED;
        // found_free_block->header = size | GET_PRE_ALLOC(found_free_block) | THIS_BLOCK_ALLOCATED;
        // PUT(FTRP(((void *)found_free_block) + 8), size | GET_PRE_ALLOC(found_free_block) | THIS_BLOCK_ALLOCATED);
        PUT(found_free_block, size | GET_PRE_ALLOC(found_free_block) | THIS_BLOCK_ALLOCATED);
        // we need to split the block
        sf_block *new_block = (sf_block *)((char *)found_free_block + size);
        // Check if new block is < 32 bytes if it is we need to add the remainder to the block
        if ((holder - GET_SIZE(found_free_block)) < 32) {
            // we set the header and footer
            size_t to_add = holder - GET_SIZE(found_free_block);
            to_add = GET_SIZE(found_free_block) + to_add;
            PUT(found_free_block, to_add | THIS_BLOCK_ALLOCATED | GET_PRE_ALLOC(found_free_block));
            PUT(FTRP(found_free_block), to_add | THIS_BLOCK_ALLOCATED | GET_PRE_ALLOC(found_free_block));
            // we need to set the next block to allocated
            sf_block *next_block = (sf_block *)((char *)found_free_block + to_add);
            sf_block *end = (sf_mem_end() - 8);
            if (next_block < end) {
                PUT(next_block, GET_SIZE(next_block) | GET_ALLOC(next_block) | PREV_BLOCK_ALLOCATED);
                PUT(FTRP(next_block), GET_SIZE(next_block) | GET_ALLOC(new_block) | PREV_BLOCK_ALLOCATED);
            } else {
                // we set the epilogue
                sf_block *epilogue = (sf_block *)((char *)sf_mem_end() - 8);
                PUT(epilogue, 0 | THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED);
            }

            // we return the pointer to the block
            return (found_free_block->body.payload);
        }
        // new_block->header = GET_SIZE(found_free_block) - size;
        PUT(new_block, (holder - GET_SIZE(found_free_block)) | PREV_BLOCK_ALLOCATED);
        PUT(FTRP((void *)new_block), (holder - GET_SIZE(found_free_block)) | PREV_BLOCK_ALLOCATED);
        // we need to add the new block to the free list
        int index_free = GET_INDEX_FREE(GET_SIZE(new_block), 32);
        if (index_free != -1) {
            sf_free_list_heads[index_free].body.links.next->body.links.prev = new_block;
            new_block->body.links.next = sf_free_list_heads[index_free].body.links.next;
            new_block->body.links.prev = &sf_free_list_heads[index_free];
            sf_free_list_heads[index_free].body.links.next = new_block;
        }
        return (void *)(found_free_block->body.payload);
    }
    sf_errno = ENOMEM;
    return NULL;
}

void sf_free(void *pp) {
    // check if pp is null
    if (pp == NULL) {
        abort();
    }
    // check if pp is aligned
    if ((uintptr_t)pp % 8 != 0) {
        abort();
    }
    // if block size is less than 32 bytes
    // size_t size_testing = GET_SIZE(((sf_block *)((char *)pp - 8)));
    // sf_block header = *((sf_block *)((char *)pp - 8));
    // size_t size_testing2 = GET_SIZE(&header);
    if (GET_SIZE(((sf_block *)((char *)pp - 8))) < 32) {
        // debug("testing1: %lu", size_testing);
        // debug("testing2: %lu", size_testing2);
        abort();
    }

    // if block size is not a multiple of 8
    if (GET_SIZE(((sf_block *)((char *)pp - 8))) % 8 != 0) {
        abort();
    }

    // check if the header is before the start of the heap or the footer is after the end of the heap
    if (((char *)pp - 8) < (char *)sf_mem_start() || ((char *)pp - 8) > (char *)sf_mem_end()) {
        abort();
    }

    // check if the block is already free
    if (GET_ALLOC(((sf_block *)((char *)pp - 8))) == 0) {
        abort();
    }

    // check if the block is in a quick list
    if (IN_QUICK((sf_block *)((char *)pp - 8)) == 4) {
        abort();
    }

    // check if the previous block is free but alloc bit is set to 1
    if (GET_PRE_ALLOC(((sf_block *)((char *)pp - 8))) == 0 && GET_ALLOC(((sf_block *)((char *)pp - 16))) == 1) {
        abort();
    }

    size_t max_quick_list_size = 32 + (NUM_QUICK_LISTS - 1) * 8;
    sf_block *block = (sf_block *)((char *)pp - 8);
    size_t size = GET_SIZE((block));

    // if it fits in the quick list it should be inserted
    if (size <= max_quick_list_size) {
        // find the index
        int index = GET_INDEX_QUICK(size, 32);
        PUT(block, size | THIS_BLOCK_ALLOCATED | GET_PRE_ALLOC(block) | IN_QUICK_LIST);

        // insert the block into the quick list if the length is not maxed
        if (sf_quick_lists[index].length < QUICK_LIST_MAX) {
            // insert the block into the quick list
            sf_quick_lists[index].length++;
            block->body.links.next = sf_quick_lists[index].first;
            sf_quick_lists[index].first = block;

            // set the header and footer
            // PUT(header, size | THIS_BLOCK_ALLOCATED | GET_PRE_ALLOC(block) | IN_QUICK_LIST);
            PUT((block), size | THIS_BLOCK_ALLOCATED | GET_PRE_ALLOC(block) | IN_QUICK_LIST);
        } else {
            // if length is max we need to flush
            if (sf_quick_lists[index].length == QUICK_LIST_MAX) {
                // flush the quick list into the free list
                for (int i = 0; i < QUICK_LIST_MAX; i++) {
                    // remove the block from the quick list
                    sf_block *removed_block = sf_quick_lists[index].first;
                    sf_block *header = removed_block;
                    // update the removed block's header
                    sf_quick_lists[index].first = sf_quick_lists[index].first->body.links.next;
                    sf_quick_lists[index].length--;
                    size_t size = GET_SIZE(header);
                    PUT(header, GET_SIZE(header) | GET_PRE_ALLOC(header));
                    sf_block *next = ((sf_block *)((char *)header + size));
                    // update next block's header
                    PUT(next, GET_SIZE(next) | GET_PRE_ALLOC(next) | GET_PREV_QUICK(next) | GET_ALLOC(next));
                    coalesce(header);
                    // sf_block *footer = (sf_block *)((char *)removed_block + GET_SIZE(removed_block) - 8);
                    // checking next block
                }
                // insert the block into the quick list

                sf_quick_lists[index].length++;
                block->body.links.next = sf_quick_lists[index].first;
                sf_quick_lists[index].first = block;

                // debug("testing");
                // sf_show_heap();
            }
        }
    } else {  // if it doesn't fit in the quick list it should be inserted into the free list
        // find the index
        int index = GET_INDEX_FREE(size, 32);
        PUT(block, size | GET_PRE_ALLOC(block));

        // define the header and footer
        sf_block *header = block;
        // sf_block *footer = (sf_block *)((char *)block + size - 8);

        // insert the block into the free list
        // checking the next block
        sf_block *next_header = (sf_block *)((char *)block + size);
        PUT(next_header, GET_SIZE(next_header) | GET_ALLOC(next_header));
        if (next_header < (sf_block *)sf_mem_end() && GET_ALLOC(next_header) == 0) {
            // first we need to free the next block from the free list
            next_header->body.links.next->body.links.prev = next_header->body.links.prev;
            next_header->body.links.prev->body.links.next = next_header->body.links.next;

            // then we need to coalesce the two blocks
            size_t next_size = GET_SIZE(next_header);
            size_t current_size = GET_SIZE(block);
            size_t new_size = next_size + current_size;
            sf_block *next_footer = (sf_block *)((char *)next_header + next_size - 8);
            sf_block *header = block;
            sf_block *footer = next_footer;
            PUT(header, new_size | GET_PRE_ALLOC(block));
            PUT(footer, new_size | GET_PRE_ALLOC(block));
            // set the next next block's prev_alloc bit
            sf_block *next_next_header = (sf_block *)((char *)next_header + next_size);
            PUT(next_next_header, GET_SIZE(next_next_header) | GET_ALLOC(next_next_header) | GET_PREV_QUICK(next_next_header));
            PUT(FTRP(next_next_header), GET_SIZE(next_next_header) | GET_ALLOC(next_next_header) | GET_PREV_QUICK(next_next_header));
        }

        // checking the prev block
        if (GET_PRE_ALLOC(block) == 0) {
            size_t prev_size = GET_SIZE(((sf_block *)((char *)block - 8)));
            // first we need to free the previous block from the free list
            sf_block *prev_header = (sf_block *)((char *)block - prev_size);
            prev_header->body.links.next->body.links.prev = prev_header->body.links.prev;
            prev_header->body.links.prev->body.links.next = prev_header->body.links.next;

            // then we need to coalesce the two blocks
            size_t current_size = GET_SIZE(block);
            size_t new_size = prev_size + current_size;
            sf_block *header = (sf_block *)((char *)block - prev_size);
            sf_block *footer = (sf_block *)((char *)block);
            PUT(header, new_size | GET_PRE_ALLOC(((sf_block *)((char *)block))));
            PUT(footer, new_size | GET_PRE_ALLOC(((sf_block *)((char *)block))));
            PUT(next_header, GET_SIZE(next_header) | GET_ALLOC(next_header) | GET_PREV_QUICK(next_header));
            PUT(FTRP(next_header), GET_SIZE(next_header) | GET_ALLOC(next_header) | GET_PREV_QUICK(next_header));
        }
        // if its neither the next or previous block is free we just need to add the footer
        if (GET_PRE_ALLOC(block) != 0 && GET_ALLOC(next_header) != 0) {
            sf_block *footer = (sf_block *)((char *)block + size - 8);
            PUT(footer, size | GET_PRE_ALLOC(block));
            PUT(header, size | GET_PRE_ALLOC(block));

            // set the next block's prev_alloc bit
            sf_block *next = (sf_block *)((char *)block + size);
            PUT(next, GET_SIZE(next) | GET_ALLOC(next) | GET_PREV_QUICK(next));
            PUT(FTRP(next), GET_SIZE(next) | GET_ALLOC(next) | GET_PREV_QUICK(next));
        }
        // add the block to the free list
        if (index != -1) {
            sf_free_list_heads[index].body.links.next->body.links.prev = header;
            header->body.links.next = sf_free_list_heads[index].body.links.next;
            header->body.links.prev = &sf_free_list_heads[index];
            sf_free_list_heads[index].body.links.next = header;
        }
    }
    // TO BE IMPLEMENTED
}

void *sf_realloc(void *pp, size_t rsize) {
    // check if pp is null
    if (pp == NULL) {
        sf_errno = EINVAL;
        return NULL;
    }
    // check if pp is aligned
    if ((uintptr_t)pp % 8 != 0) {
        sf_errno = EINVAL;
        return NULL;
    }
    // if block size is less than 32 bytes
    if (GET_SIZE(((sf_block *)((char *)pp - 8))) < 32) {
        sf_errno = EINVAL;
        return NULL;
    }

    // if block size is not a multiple of 8
    if (GET_SIZE(((sf_block *)((char *)pp - 8))) % 8 != 0) {
        sf_errno = EINVAL;
        return NULL;
    }

    // check if the header is before the start of the heap or the footer is after the end of the heap
    if (((char *)pp - 8) < (char *)sf_mem_start() || ((char *)pp - 8) > (char *)sf_mem_end()) {
        sf_errno = EINVAL;
        return NULL;
    }

    // check if the block is already free
    if (GET_ALLOC(((sf_block *)((char *)pp - 8))) == 0) {
        sf_errno = EINVAL;
        return NULL;
    }

    // check if the block is in a quick list
    if (IN_QUICK((sf_block *)((char *)pp - 8)) == 4) {
        sf_errno = EINVAL;
        return NULL;
    }

    // check if the previous block is free but alloc bit is set to 1
    if (GET_PRE_ALLOC(((sf_block *)((char *)pp - 8))) == 0 && GET_ALLOC(((sf_block *)((char *)pp - 16))) == 1) {
        sf_errno = EINVAL;
        return NULL;
    }

    // if pointer is valid but size is 0 we should free the block
    if (rsize == 0) {
        sf_free(pp);
        return NULL;
    }

    sf_block *block = (sf_block *)((char *)pp - 8);
    // check if the size of the block is larger than the size given
    if (GET_SIZE(block) < rsize) {
        // call sf_malloc to get a larger block
        void *new_block = sf_malloc(rsize);
        // if the new block is null return null CHECK IF THIS IS RIGHT
        if (new_block == NULL) {
            return NULL;
        }
        // copy the data from the old block to the new block
        memcpy(new_block, pp, rsize);
        // free the old block
        sf_free(pp);
        return new_block;
    }
    // if the size of the block is smaller than the size given
    if (GET_SIZE(block) > rsize) {
        // attempt to split the block
        size_t min = 32;
        size_t size = GET_SIZE(block);
        size_t new_size = rsize + 8;
        if (new_size % 8 != 0) {
            new_size = new_size + (8 - (new_size % 8));
        }
        if (new_size < min) {
            new_size = min;
        }
        size_t remainder = size - new_size;
        // if the remainder is less than the minimum block size we cannot split the block
        if (remainder < min) {
            return pp;
        }
        // if the remainder is greater than the minimum block size we can split the block
        if (remainder >= min) {
            // set the header of the old block
            PUT(block, new_size | GET_PRE_ALLOC(block) | THIS_BLOCK_ALLOCATED);
            // get the header of the remainder block
            sf_block *remainder_header = (sf_block *)((char *)block + new_size);
            // set the header of the remainder block
            PUT(remainder_header, remainder | PREV_BLOCK_ALLOCATED);
            // coalesce the remainder block
            coalesce(remainder_header);
            return pp;
        }
    }
    return pp;
}

void *sf_memalign(size_t size, size_t align) {
    // check if align is at least 8 and align is a power of 2
    if (align < 8 || (align & (align - 1)) != 0) {
        sf_errno = EINVAL;
        return NULL;
    }

    // check if size is 0
    if (size == 0) {
        return NULL;
    }
    // allocating a space that is larger than the requested size
    size_t larger = size + align + 32 + 8;
    void *block = sf_malloc(larger);
    if (block == NULL) {
        return NULL;
    }
    // BLOCK IS THE PAYLOAD NOT THE HEADER

    // if the payload satifies the alignment
    if ((uintptr_t)block % align == 0) {
        size_t original = GET_SIZE(block - 8);
        // check the size and see if we can split the block
        if ((original - size) >= 32) {
            // set the header of the block
            size_t block_size = size + 8;
            if (block_size < 32) {
                block_size = 32;
            }
            PUT(block - 8, (block_size) | THIS_BLOCK_ALLOCATED | GET_PRE_ALLOC(block - 8));  // CHECKED
            // set the header of the remainder block
            sf_block *remainder_header = (sf_block *)((char *)block + block_size - 8);
            PUT(remainder_header, (original - block_size) | PREV_BLOCK_ALLOCATED | THIS_BLOCK_ALLOCATED);  // CHECKED
            coalesce(remainder_header);
            // coalesce the remainder block
            // coalesce(block + size - 8);
            return block;
        }
        // if the size is less than 32 we cannot split the block
        PUT(block - 8, original | THIS_BLOCK_ALLOCATED | GET_PRE_ALLOC(block - 8));  // CHECKED
        // sf_block *end = (sf_block *)((char *)block - 8 + size);
        // PUT(end, (original - size) | PREV_BLOCK_ALLOCATED);
        return block;
    } else {
        // if the payload doesn't satisfy the alignment
        void *changed_block = (block + 32);
        // if the payload doesn't satisfy the alignment
        size_t offset = (uintptr_t)align - ((uintptr_t)changed_block % align);
        void *new_block = ((char *)changed_block + offset);
        size_t original = GET_SIZE(block - 8);
        size_t block_size = size + 8;
        if (block_size < 32) {
            block_size = 32;
        }
        PUT(new_block - 8, (block_size) | THIS_BLOCK_ALLOCATED);

        // free left side
        PUT(block - 8, (offset + 32) | GET_PRE_ALLOC(block - 8) | GET_ALLOC(block - 8));
        coalesce(block - 8);
        // coalesce the free block
        // coalesce(block - 8);

        // free right side if there is enough space
        if ((original - block_size - offset - 32) >= 32) {
            sf_block *end = (sf_block *)((char *)new_block - 8 + block_size);
            PUT(end, (original - block_size - offset - 32) | PREV_BLOCK_ALLOCATED | THIS_BLOCK_ALLOCATED);
            coalesce(end);

            // coalesce the free block
            // coalesce(end - 8);
        } else {
            PUT(new_block - 8, (original - offset - 32) | THIS_BLOCK_ALLOCATED);
        }
        return new_block;
    }
}