//
// Copyright (c) 2011-2023, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

use std::path::PathBuf;

lazy_static::lazy_static! {
    static ref METADATA: native_file_tests::TestMetadata = {
        let nft_path = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("..")
            .join("libudi")
            .join("native-file-tests");

        native_file_tests::create_test_metadata(&nft_path, std::env::consts::OS)
            .expect("Failed to create test metadata")
    };
}

pub fn get_test_metadata() -> &'static native_file_tests::TestMetadata {
    &METADATA
}
