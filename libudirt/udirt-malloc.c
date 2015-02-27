/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "udirt.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

// implementation of a simple, non-threadsafe heap implementation, specific to UDI RT

static
const size_t ALLOC_SIZE = 16384; // 16KB

static
unsigned long global_max_size = 10485760; // 10 MB default

static
const int TRACING = 0; // set to 1 to enable heap tracing, useful for debugging memory corruption

#define trace(format, ...) \
    do {\
        if( TRACING ) {\
            fprintf(stderr, "%s[%d]: " format, __FILE__, __LINE__,\
                    ## __VA_ARGS__);\
        }\
    }while(0)


// Terms used in this implementation
// Heap    - collection of segments
// Segment - collection of chunks
// Chunk - unit of allocation

// Here is the layout used by the heap:
// 
// | segment | chunk | user data | chunk | user data | ... | segment |

typedef struct segment_struct segment;

typedef struct chunk_struct {
    unsigned short size;
    struct chunk_struct *next_chunk;
    segment *parent_segment;
} chunk;

struct segment_struct {
    unsigned int free_space;
    chunk *free_list;
    struct segment_struct *next_segment;
    struct segment_struct *prev_segment;
};

typedef struct heap_structure {
    unsigned int num_segments;
    segment *segment_list;
    segment *last_segment;
    unsigned int num_frees;
    unsigned int num_mallocs;
} heap_struct;

static heap_struct heap = { 0, NULL, NULL, 0, 0 };

/**
 * Sets the maximum size of the heap
 *
 * @param max_size the maximum size to set (rounded up to the size of chunk
 * allocated from the OS)
 */
void udi_set_max_mem_size(unsigned long max_size) {
    unsigned long tmp_max_size = max_size;

    // Round up to the nearest alloc size
    unsigned long offset = 0;
    if ( ((ALLOC_SIZE-1) & tmp_max_size) ) {
        offset += ALLOC_SIZE;
    }
    tmp_max_size = (tmp_max_size & ~(ALLOC_SIZE-1));
    tmp_max_size += offset;

    global_max_size = max_size;
}

/**
 * Allocates a segment by asking the OS for mapped memory
 *
 * @return the new segment, NULL on failure
 */
static
segment *allocate_segment() {
    // Don't count overhead of segments
    if ( (heap.num_segments+1) * ALLOC_SIZE > global_max_size ) {
        trace("Could not allocate segment, maximum number of segments (%d) already allocated\n",
                heap.num_segments);
        return NULL;
    }

    segment *ret = (segment *)map_mem(ALLOC_SIZE + sizeof(segment));
    if ( ret == NULL ) return NULL;

    segment new_segment;
    new_segment.free_space = ALLOC_SIZE;
    new_segment.next_segment = NULL;
    new_segment.prev_segment = NULL;

    // Allocate the first free chunk
    chunk initial_chunk;
    initial_chunk.next_chunk = NULL;
    initial_chunk.parent_segment = ret;
    initial_chunk.size = new_segment.free_space - sizeof(chunk);
    memcpy(((unsigned char *)ret) + sizeof(segment), &initial_chunk, sizeof(chunk));

    new_segment.free_list = (chunk *)(((unsigned char *)ret) + sizeof(segment));

    memcpy(ret, &new_segment, sizeof(segment));

    // Add the new segment to the heap
    if ( heap.last_segment == NULL ) {
        heap.segment_list = ret;
        heap.last_segment = ret;
        heap.num_segments = 1;
    }else{
        heap.last_segment->next_segment = ret;
        ret->prev_segment = heap.last_segment;
        heap.last_segment = ret;
        heap.num_segments++;
    }

    trace("allocated new segment at %p\n", ret);

    return ret;
}

void check_free_space(const char *file, int line) {

    segment *seg_ptr = heap.segment_list;
    while (seg_ptr != NULL) {
        unsigned int actual_free_space = 0;
        
        chunk *chunk_ptr = seg_ptr->free_list;
        while (chunk_ptr != NULL) {
            actual_free_space += chunk_ptr->size + sizeof(chunk);

            chunk_ptr = chunk_ptr->next_chunk;
        }

        if ( actual_free_space != seg_ptr->free_space) {
            dump_heap();
            udi_printf("Inconsistent free space for segment %p\n", seg_ptr);
            udi_abort(file, line);
        }

        seg_ptr = seg_ptr->next_segment;
    }
}

/**
 * Internal UDI RT heap free
 */
void udi_free(void *in_ptr) {
    if (in_ptr == NULL || heap.segment_list == NULL) return;

    chunk *chunk_ptr = (chunk *)(((unsigned char *)in_ptr) - sizeof(chunk));
    heap.num_frees++;

    segment *parent_segment = chunk_ptr->parent_segment;

    unsigned long chunk_addr = (unsigned long)chunk_ptr;

    // Return space to segment pool
    parent_segment->free_space += chunk_ptr->size + sizeof(chunk);

    trace("deallocating chunk of size %d at %p, free space %u in %p\n", chunk_ptr->size, chunk_ptr,
            parent_segment->free_space, parent_segment);

    if ( parent_segment->free_space == ALLOC_SIZE ) {
        // This segment has been free'd completely, give back to the OS

        if ( parent_segment->next_segment != NULL ) {
            parent_segment->next_segment->prev_segment = parent_segment->prev_segment;
        }

        if ( parent_segment->prev_segment != NULL ) {
            parent_segment->prev_segment->next_segment = parent_segment->next_segment;
        }

        if ( heap.last_segment == parent_segment ) {
            heap.last_segment = parent_segment->prev_segment;
        }

        if ( heap.segment_list == parent_segment ) {
            heap.segment_list = parent_segment->next_segment;
        }
        
        if ( unmap_mem(parent_segment, ALLOC_SIZE + sizeof(segment)) ) {
            udi_abort(__FILE__, __LINE__);
        }

        trace("deallocated segment at %p\n", parent_segment);

        heap.num_segments--;

        if ( TRACING ) {
            check_free_space(__FILE__, __LINE__);
        }
        
        return;
    }

    // The segment was completely utilized, need to re-establish a free list
    if (parent_segment->free_list == NULL) {
        parent_segment->free_list = chunk_ptr;

        if ( TRACING ) {
            check_free_space(__FILE__, __LINE__);
        }

        return;
    }

    // Need to add the chunk to the free list
    chunk *tmp = parent_segment->free_list;
    chunk *last_chunk = tmp;
    while (tmp != NULL && (chunk_addr > ((unsigned long)tmp))) {
        last_chunk = tmp;
        tmp = tmp->next_chunk;
    }

    if ( tmp == NULL && last_chunk == NULL ) {
        dump_heap();
        udi_abort(__FILE__, __LINE__);
        return;
    }

    // Case 1: the free'd chunk needs to be added to the end of the free list
    if ( tmp == NULL && last_chunk != NULL ) {
        last_chunk->next_chunk = chunk_ptr;

        if ( TRACING ) {
            check_free_space(__FILE__, __LINE__);
        }

        return;
    }
    
    unsigned long tmp_addr = (unsigned long)tmp;
    unsigned long last_addr = (unsigned long)last_chunk;

    // Case 2: free'd chunk is adjacent to free chunk before and after it
    if (    (tmp_addr - sizeof(chunk) - chunk_ptr->size) == chunk_addr
         && (last_addr + sizeof(chunk) + last_chunk->size) == chunk_addr )
    {
        last_chunk->next_chunk = tmp->next_chunk;
        last_chunk->size += chunk_ptr->size + tmp->size + sizeof(chunk) + sizeof(chunk);
    // Case 3: free'd chunk is adjacent to free chunk before it
    }else if ( (last_addr + sizeof(chunk) + last_chunk->size) == chunk_addr ) {
        last_chunk->size += sizeof(chunk) + chunk_ptr->size;
    // Case 4: free'd chunk is adjacent to free chunk after it
    }else if ( (tmp_addr - sizeof(chunk) - chunk_ptr->size) == chunk_addr ) {
        chunk_ptr->size += sizeof(chunk) + tmp->size;
        if ( tmp == parent_segment->free_list ) {
            parent_segment->free_list = chunk_ptr;
            chunk_ptr->next_chunk = tmp->next_chunk;
        }else{
            last_chunk->next_chunk = chunk_ptr;
            chunk_ptr->next_chunk = tmp->next_chunk;
        }
    // Case 5: free'd chunk is before the first chunk in the free list
    }else if ( ((unsigned long)parent_segment->free_list) > chunk_addr ) {
        chunk_ptr->next_chunk = parent_segment->free_list;
        parent_segment->free_list = chunk_ptr;
    // Case 6: free'd chunk is not adjacent to any free chunk
    }else{
        last_chunk->next_chunk = chunk_ptr;
        chunk_ptr->next_chunk = tmp;
    }

    if ( TRACING ) {
        check_free_space(__FILE__, __LINE__);
    }

}

/**
 * Internal UDI RT heap allocate
 *
 * @param length the size in bytes of the allocation
 *
 * @return the allocated memory, NULL on failure
 */
void *udi_malloc(size_t length) {
    if ( length == 0 ) return NULL;

    if ( length > (ALLOC_SIZE - sizeof(chunk)) ) {
        errno = ENOMEM;
        trace("Cannot allocate chunk of size %lu\n", length);
        return NULL;
    }

    segment *tmp_head = heap.segment_list;

    do {
        // Search existing segments for suitable memory
        segment *tmp_segment = tmp_head;
        while ( tmp_segment != NULL 
                && tmp_segment->free_space < (length + sizeof(chunk)) )
        {
            tmp_segment = tmp_segment->next_segment;
        }

        // If the search failed, create a new block
        if ( tmp_segment == NULL ) {
            tmp_segment = allocate_segment();
            if ( tmp_segment == NULL ) {
                errno = ENOMEM;
                return NULL;
            }

            // Handle initial allocation
            if (tmp_head == NULL) {
                tmp_head = tmp_segment;
            }
        }

        // Now search the segments free_list for a suitable free chunk
        chunk *tmp_chunk = tmp_segment->free_list;
        chunk *last_chunk = tmp_chunk;
        while (tmp_chunk != NULL && tmp_chunk->size < length) {
            last_chunk = tmp_chunk;
            tmp_chunk = tmp_chunk->next_chunk;
        }

        // If there wasn't a suitable free chunk due to fragmentation keep trying
        if ( tmp_chunk == NULL ) {
            tmp_head = tmp_segment->next_segment;
            continue;
        }

        // Case 1: the found chunk is the exact size requested or the found
        // chunk is greater than size requested but there is not enough space
        // left over for another chunk

        if (    tmp_chunk->size == length 
             || ((tmp_chunk->size - length) < (sizeof(chunk) + 1)) ) 
        {
            if ( tmp_chunk == tmp_segment->free_list ) {
                tmp_segment->free_list = tmp_chunk->next_chunk;
            }else{
                last_chunk->next_chunk = tmp_chunk->next_chunk;
            }
            tmp_segment->free_space -= (sizeof(chunk) + tmp_chunk->size);
        // Case 2: the found chunk is greater than size requested and there is
        // enough space left over for another chunk
        }else{
            // create new chunk and allocate tmp_chunk
            chunk new_chunk;
            new_chunk.size = tmp_chunk->size - length - sizeof(chunk);
            new_chunk.parent_segment = tmp_segment;
            new_chunk.next_chunk = tmp_chunk->next_chunk;

            tmp_chunk->size = length;

            unsigned long new_chunk_addr = ((unsigned long)tmp_chunk) + sizeof(chunk) + length;
            memcpy((void *)new_chunk_addr, &new_chunk, sizeof(chunk));

            // link in new chunk
            if ( tmp_chunk != last_chunk ) {
                last_chunk->next_chunk = (chunk *)new_chunk_addr;
            }

            if ( tmp_chunk == tmp_segment->free_list ) {
                tmp_segment->free_list = (chunk *)new_chunk_addr;
            }

            tmp_segment->free_space -= (sizeof(chunk) + tmp_chunk->size);
        }

        trace("allocated chunk of size %hu at %p, free space %u in segment %p\n", tmp_chunk->size, tmp_chunk,
                tmp_segment->free_space, tmp_segment);
        
        heap.num_mallocs++;

        if ( TRACING ) {
            check_free_space(__FILE__, __LINE__);
        }

        tmp_chunk->next_chunk = NULL;

        return (void *)(((unsigned char *)tmp_chunk) + sizeof(chunk));
    }while(1);
}

/** Debugging function */
void dump_heap() {
    fprintf(stderr, "Num segments: %u\n", heap.num_segments);
    fprintf(stderr, "Last segment: %p\n", heap.last_segment);
    fprintf(stderr, "Number of mallocs: %u\n", heap.num_mallocs);
    fprintf(stderr, "Number of frees: %u\n", heap.num_frees);
    fprintf(stderr, "\n");

    segment *seg_ptr = heap.segment_list;
    while (seg_ptr != NULL) {
        fprintf(stderr, "Segment: %p\n", seg_ptr);
        fprintf(stderr, "Next segment: %p\n", seg_ptr->next_segment);
        fprintf(stderr, "Prev segment: %p\n", seg_ptr->prev_segment);
        fprintf(stderr, "Free space: %u\n", seg_ptr->free_space);
        fprintf(stderr, "Free list: %p\n", seg_ptr->free_list);

        chunk *chunk_ptr =  seg_ptr->free_list;
        while (chunk_ptr != NULL) {
            fprintf(stderr, "\tChunk: %p\n", chunk_ptr);
            fprintf(stderr, "\tParent segment: %p\n", chunk_ptr->parent_segment);
            fprintf(stderr, "\tSize: %u\n", chunk_ptr->size);
            fprintf(stderr, "\tNext chunk: %p\n", chunk_ptr->next_chunk);
            fprintf(stderr, "\n");

            chunk_ptr = chunk_ptr->next_chunk;
        }

        fprintf(stderr, "\n");

        seg_ptr = seg_ptr->next_segment;
    }
}

