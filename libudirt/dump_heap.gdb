# Copyright (c) 2011-2015, UDI Contributors
# All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

define udi-dump-heap
    printf "Num segments: %d\n", heap.num_segments
    printf "Last segment: %p\n", heap.last_segment
    printf "\n"

    set $seg_ptr = heap.segment_list
    while $seg_ptr != 0
        printf "Segment: %p\n", $seg_ptr
        printf "Next segment: %p\n", $seg_ptr->next_segment
        printf "Prev segment: %p\n", $seg_ptr->prev_segment
        printf "Free space: %d\n", $seg_ptr->free_space
        printf "Free list: %p\n", $seg_ptr->free_list

        set $chunk_ptr = $seg_ptr->free_list
        while $chunk_ptr != 0
            printf "\tChunk: %p\n", $chunk_ptr
            printf "\tParent segment: %p\n", $chunk_ptr->parent_segment
            printf "\tSize: %d\n", $chunk_ptr->size
            printf "\tNext chunk: %p\n", $chunk_ptr->next_chunk

            set $chunk_ptr = $chunk_ptr->next_chunk
        end
        
        set $seg_ptr = $seg_ptr->next_segment
    end
end

document udi-dump-heap
    Dumps the udi heap
end

# vim: set syntax=gdb:
