/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LIB_UDI_TEST_H
#define LIB_UDI_TEST_H

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
