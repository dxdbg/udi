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

// implementation of a simple heap implementation, specific to UDI RT

static
const size_t ALLOC_SIZE = 16384; // 16KB

static
unsigned long global_max_size = 1048576; // 1 MB default

// Terms used in this implementation
// Heap    - collection of segments
// Segment - collection of chunks
// Chunk - unit of allocation

typedef struct chunk_struct {
    unsigned char allocated;
    unsigned short size;
    chunk *next_chunk;
    segment *parent_segment;
} chunk;

typedef struct segment_struct {
    unsigned int free_space;
    chunk *free_list;
    struct segment_struct *next_segment;
} segment

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
    segment *ret = (segment *)map_mem(ALLOC_SIZE + sizeof(segment));
    if ( ret == NULL ) return NULL;

    segment new_segment;
    new_segment.free_space = ALLOC_SIZE;
    new_segment.next_segment = NULL;

    // Allocate the first free chunk
    chunk initial_chunk;
    initial_chunk.allocated = 0;
    initial_chunk.next_chunk = NULL;
    initial_chunk.parent_segment = ret;
    initial_chunk.size = ALLOC_SIZE - sizeof(chunk);
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
        heap.last_segment = ret;
        heap.num_segments++;
    }

    return ret;
}

void udi_free(void *in_ptr) {
    if (in_ptr == NULL || heap.segment_list == NULL) return;

    chunk *chunk_ptr = (chunk *)(((unsigned char *)in_ptr) - sizeof(chunk));

    unsigned long chunk_addr = (unsigned long)chunk_ptr;

    chunk_ptr->allocated = 0;

    // Need to add the chunk to the free list
    chunk *tmp = chunk_ptr->parent_segment->free_list;
    chunk *last_chunk = tmp;
    while (tmp != NULL && (chunk_addr < ((unsigned long)tmp))) {
        last_chunk = tmp;
        tmp = tmp->next_chunk;
    }

    // This should never happen, but silently fail if it does
    if ( tmp == NULL ) return;

    unsigned long tmp_addr = (unsigned long)tmp;
    unsigned long last_addr = (unsigned long)last_chunk;

    // Case 1: free'd chunk is adjacent to free chunk before and after it
    // Case 2: free'd chunk is adjacent to free chunk before it
    // Case 3: free'd chunk is adjacent to free chunk after it
    // Case 4: free'd chunk is not adjacent to any free chunk

    // TODO start here
}

void *udi_malloc(size_t length) {
    if ( length > ALLOC_SIZE ) {
        errno = ENOMEM;
        return NULL;
    }

    unsigned char *ret = NULL;

    // Search existing segments for suitable memory
    segment *tmp_segment = heap.segment_list;
    while ( tmp_segment != NULL ) {

    }

    // If the search failed, create a new block

    return (void *)ret;
}
