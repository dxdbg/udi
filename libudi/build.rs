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

fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let manifest_path = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());

    let nft_path = manifest_path.join("native-file-tests");

    if nft_path.exists() {
        native_file_tests::setup(&nft_path,
                                 &out_path,
                                 &env::var("CARGO_CFG_TARGET_OS").unwrap());
    }
}
