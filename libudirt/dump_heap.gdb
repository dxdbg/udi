# Copyright (c) 2011-2013, Dan McNulty
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    # Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    # Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    # Neither the name of the UDI project nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
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
