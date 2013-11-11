/*
 * Copyright (c) 2011-2013, Dan McNulty
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

#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include "udirt.h"
#include "libuditest.h"

using std::cout;
using std::endl;

extern "C" {
extern void check_free_space(const char *file, int line);
}

class test_malloc : public UDITestCase {
    public:
        test_malloc()
            : UDITestCase(std::string("test_malloc")) {}
        virtual ~test_malloc() {}
        
        bool operator()(void);
};

static test_malloc testInstance;

bool basic_test() {
    int **ptr = (int **)udi_malloc(sizeof(int *)*10);

    int i;
    for (i = 0; i < 10; ++i) {
        ptr[i] = (int *)udi_malloc(sizeof(int));
        *(ptr[i]) = i;
    }

    for (i = 0; i < 10; ++i) {
        if ( *(ptr[i]) != i ) {
            cout << "Unexpected value in integer array: ( "
                 << *(ptr[i]) << " !=  " << i << endl;
            return false;
        }
    }

    for (i = 0; i < 10; ++i) {
        udi_free(ptr[i]);
    }
    udi_free(ptr);

    check_free_space(__FILE__, __LINE__);

    return true;
}

static
void set_all_chars(char *str, int length) {
    char value = (char)length;

    int i;
    for (i = 0; i < length; ++i) {
        str[i] = value;
    }
}

bool fragment_test() {
    const int malloc_size = 16;

    char *last_alloc = (char *)udi_malloc(sizeof(char)*malloc_size);
    set_all_chars(last_alloc, malloc_size);

    char *current_alloc = (char *)udi_malloc(sizeof(char)*malloc_size);
    set_all_chars(current_alloc, malloc_size);

    int i;
    for (i = 1;  i < 10; ++i) {
        udi_free(last_alloc);

        char value = (char)(i*malloc_size);

        int j;
        for (j = 0; j < (i*malloc_size); ++j) {
            if ( current_alloc[j] != value ) {
                cout << "Unexpected value found in allocated string ( "
                     << (unsigned int)(current_alloc[j]) << " != "
                     << (unsigned int)(value) << " )" << endl;
                return false;
            }
        }

        last_alloc = current_alloc;
        current_alloc = (char *)udi_malloc(sizeof(char)*malloc_size*(i+1));
        set_all_chars(current_alloc, malloc_size*(i+1));
    }

    udi_free(last_alloc);
    udi_free(current_alloc);

    check_free_space(__FILE__, __LINE__);

    return true;
}

bool coalesce_test() {

    void *one = udi_malloc(1144);
    void *two = udi_malloc(32);
    void *three = udi_malloc(41);
    void *four = udi_malloc(40);

    udi_free(two);
    check_free_space(__FILE__, __LINE__);

    udi_free(three);
    check_free_space(__FILE__, __LINE__);

    udi_free(four);
    check_free_space(__FILE__, __LINE__);

    two = udi_malloc(16);
    three = udi_malloc(2);
    four = udi_malloc(2);

    udi_free(four);
    check_free_space(__FILE__, __LINE__);

    udi_free(three);
    check_free_space(__FILE__, __LINE__);

    udi_free(two);
    check_free_space(__FILE__, __LINE__);

    udi_free(one);
    check_free_space(__FILE__, __LINE__);

    return true;
}

bool test_malloc::operator()(void) {

    bool result = basic_test();

    if ( !result ) return result;

    result = fragment_test();

    if ( !result ) return result;

    return coalesce_test();
}
