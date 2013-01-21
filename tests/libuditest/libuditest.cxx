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

#include "libuditest.h"

#include <vector>
#include <cstdlib>
#include <iostream>
#include <string>

using std::vector;
using std::cout;
using std::endl;
using std::string;

/** Globally maintained list of test cases */
vector<UDITestCase *> UDITestCase::testCases;

/**
 * Constructor.
 *
 * @param testName      the name of this test
 */
UDITestCase::UDITestCase(const std::string &testName) 
    : testName(testName)
{
    testCases.push_back(this);
}

/**
 * Destructor
 */
UDITestCase::~UDITestCase() {
    // Don't actually do anything because all allocation done statically
}

/**
 * Executes all registered test cases
 *
 * @param argc  the argc argument to the main function (currently unused)
 * @param argv  the argv argument to the main function (currently unused)
 */
int UDITestCase::executeTests(int /*argc*/, char ** /*argv[]*/) {
    for (vector<UDITestCase *>::iterator i = testCases.begin();
            i != testCases.end(); ++i)
    {
        UDITestCase *currentCase = *i;
        bool testResult = false;

        try {
            testResult = (*currentCase)();
        }catch (const UDITestError &e) {
            cout << currentCase->name() 
                 << " failed with exception at " 
                 << e.getFile()
                 << ":"
                 << e.getLine();

            if (!string(e.what()).empty()) {
                cout << " -- "
                     << e.what();
            }

            cout << endl;

            testResult = false;
            return EXIT_FAILURE;
        }

        if (!testResult) {
            cout << currentCase->name() << " failed" << endl;
            return EXIT_FAILURE;
        }

        cout << currentCase->name() << " passed" << endl;
    }

    return EXIT_SUCCESS;
}

/**
 * @return the name for this test case
 */
const std::string &UDITestCase::name() const {
    return testName;
}
