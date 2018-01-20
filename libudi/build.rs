//
// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]

extern crate native_file_tests;

use std::env;
use std::path::PathBuf;
use std::process::Command;

const NATIVE_FILE_TESTS_URL: &'static str =
    "https://github.com/dxdbg/native-file-tests/releases/download/v0.1.4/native-file-tests-v0.1.4.zip";

fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let manifest_path = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());

    let local_zip = manifest_path.join("native-file-tests.zip");

    if !local_zip.exists() {
        Command::new("curl").arg("-sSfL")
                            .arg("-o")
                            .arg(local_zip.to_str().unwrap())
                            .arg(NATIVE_FILE_TESTS_URL)
                            .spawn()
                            .expect("Failed to start download of native file tests zip")
                            .wait()
                            .expect("Failed to download native file tests zip");
    }

    native_file_tests::setup(&manifest_path,
                             &out_path,
                             &local_zip,
                             &env::var("CARGO_CFG_TARGET_OS").unwrap());
}
