/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
