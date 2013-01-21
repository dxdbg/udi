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

#ifndef LIB_UDI_TEST_H
#define LIB_UDI_TEST

#include <vector>
#include <string>
#include <stdexcept>

/**
 * Abstract class that represents a test case.
 *
 * Provides global tracking of all created test cases
 */
class UDITestCase {
    private:
        static std::vector<UDITestCase *> testCases;
    protected:
        const std::string testName;

    public:
        UDITestCase(const std::string &testName);
        virtual ~UDITestCase();

        static int executeTests(int argc, char *argv[]);

        virtual bool operator()(void) = 0;
        virtual const std::string &name() const;
};

/**
 * Exception thrown to terminate a test
 */
class UDITestError : public std::runtime_error {
    public:
        UDITestError(const char *file, int line, const char *msg = "")
           : std::runtime_error(msg), file(file), line(line)
        {}

        const char *getFile() const { return file; }
        int getLine() const { return line; }

    private:
        const char *file;
        int line;
};

#define test_assert_msg(s, b) udi_test_assert(__FILE__, __LINE__, s, b)
#define test_assert(b) udi_test_assert(__FILE__, __LINE__, b)

/**
 * Tests an assertion in a test, failing the test if the assertion is not true
 *
 * @param condition the condition for the assertion
 */
inline
void udi_test_assert(const char *file, int line, const char *msg, bool condition) {
    if ( !condition ) throw UDITestError(file, line, msg);
}

/**
 * Tests an assertion in a test, failing the test if the assertion is not true
 *
 * @param condition the condition for the assertion
 */
inline
void udi_test_assert(const char *file, int line, bool condition) {
    if ( !condition ) throw UDITestError(file, line);
}

#endif
