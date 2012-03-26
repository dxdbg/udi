/*
 * Copyright (c) 2011-2012, Dan McNulty
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the UDI project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
} heap_struct;

static heap_struct heap = { 0, NULL, NULL};

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

static
segment *allocate_segment() {
    // Don't count overhead of segments
    if ( (heap.num_segments+1) * ALLOC_SIZE > global_max_size ) {
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
    initial_chunk.size = new_segment.free_space;
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

    return ret;
}

void udi_free(void *in_ptr) {
    if (in_ptr == NULL || heap.segment_list == NULL) return;

    chunk *chunk_ptr = (chunk *)(((unsigned char *)in_ptr) - sizeof(chunk));

    segment *parent_segment = chunk_ptr->parent_segment;

    unsigned long chunk_addr = (unsigned long)chunk_ptr;

    // Return space to segment pool
    parent_segment->free_space += chunk_ptr->size + sizeof(chunk);

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

        heap.num_segments--;
        
        return;
    }


    // Need to add the chunk to the free list
    chunk *tmp = parent_segment->free_list;
    chunk *last_chunk = tmp;
    while (tmp != NULL && (chunk_addr > ((unsigned long)tmp))) {
        last_chunk = tmp;
        tmp = tmp->next_chunk;
    }

    if ( tmp == NULL ) {
        udi_abort(__FILE__, __LINE__);
        return;
    }
    
    unsigned long tmp_addr = (unsigned long)tmp;
    unsigned long last_addr = (unsigned long)last_chunk;

    // Case 1: free'd chunk is adjacent to free chunk before and after it
    if (    (tmp_addr - sizeof(chunk) - chunk_ptr->size) == chunk_addr
         && (last_addr + sizeof(chunk) + last_chunk->size) == chunk_addr )
    {
        last_chunk->next_chunk = tmp->next_chunk;
        last_chunk->size = chunk_ptr->size + tmp->size + sizeof(chunk) + sizeof(chunk);
    // Case 2: free'd chunk is adjacent to free chunk before it
    }else if ( (last_addr + sizeof(chunk) + last_chunk->size) == chunk_addr ) {
        last_chunk->size += sizeof(chunk) + chunk_ptr->size;
    // Case 3: free'd chunk is adjacent to free chunk after it
    }else if ( (tmp_addr - sizeof(chunk) - chunk_ptr->size) == chunk_addr ) {
        chunk_ptr += sizeof(chunk) + tmp->size;
        if ( tmp == parent_segment->free_list ) {
            parent_segment->free_list = chunk_ptr;
        }else{
            last_chunk->next_chunk = chunk_ptr;
            chunk_ptr->next_chunk = tmp->next_chunk;
        }
    // Case 4: free'd chunk is not adjacent to any free chunk
    }else{
        last_chunk->next_chunk = chunk_ptr;
        chunk_ptr->next_chunk = tmp;
    }
}

void *udi_malloc(size_t length) {
    if ( length > (ALLOC_SIZE - sizeof(chunk)) ) {
        errno = ENOMEM;
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
            last_chunk->next_chunk = tmp_chunk->next_chunk;
            tmp_segment->free_space -= sizeof(chunk) + tmp_chunk->size;
        // Case 2: the found chunk is greater than size requested and there is
        // enough space left over for another chunk
        }else{
            // create new chunk and allocate tmp_chunk
            chunk new_chunk;
            new_chunk.size = tmp_chunk->size - length - sizeof(chunk);
            new_chunk.parent_segment = tmp_segment;
            new_chunk.next_chunk = tmp_chunk->next_chunk;

            unsigned long new_chunk_addr = ((unsigned long)tmp_chunk) + sizeof(chunk) + length;
            memcpy((void *)new_chunk_addr, &new_chunk, sizeof(chunk));

            // link in new chunk
            if ( tmp_chunk != last_chunk ) {
                last_chunk->next_chunk = (chunk *)new_chunk_addr;
            }

            if ( tmp_chunk == tmp_segment->free_list ) {
                tmp_segment->free_list = (chunk *)new_chunk_addr;
            }

            tmp_segment->free_space -= sizeof(chunk) + new_chunk.size;
        }
        
        return (void *)(((unsigned char *)tmp_chunk) + sizeof(chunk));
    }while(1);
}

/** Debugging function */
void dump_heap() {
    fprintf(stderr, "Num segments: %d\n", heap.num_segments);
    fprintf(stderr, "Last segment: %p\n", heap.last_segment);

    segment *seg_ptr = heap.segment_list;
    while (seg_ptr != NULL) {
        fprintf(stderr, "Segment: %p\n", seg_ptr);
        fprintf(stderr, "Next segment: %p\n", seg_ptr->next_segment);
        fprintf(stderr, "Prev segment: %p\n", seg_ptr->prev_segment);
        fprintf(stderr, "Free space: %d\n", seg_ptr->free_space);
        fprintf(stderr, "Free list: %p\n", seg_ptr->free_list);

        chunk *chunk_ptr =  seg_ptr->free_list;
        while (chunk_ptr != NULL) {
            fprintf(stderr, "\tChunk: %p\n", chunk_ptr);
            fprintf(stderr, "\tParent segment: %p\n", chunk_ptr->parent_segment);
            fprintf(stderr, "\tSize: %d\n", chunk_ptr->size);
            fprintf(stderr, "\tNext chunk: %p\n", chunk_ptr->next_chunk);

            chunk_ptr = chunk_ptr->next_chunk;
        }

        seg_ptr = seg_ptr->next_segment;
    }
}
