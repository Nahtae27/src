// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_tests/extension_manifest_test.h"

#include "chrome/common/extensions/extension_manifest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using extensions::Extension;

namespace errors = extension_manifest_errors;

TEST_F(ExtensionManifestTest, UpdateUrls) {
  // Test several valid update urls
  Testcase testcases[] = {
    Testcase("update_url_valid_1.json", Extension::INTERNAL,
             Extension::STRICT_ERROR_CHECKS),
    Testcase("update_url_valid_2.json", Extension::INTERNAL,
             Extension::STRICT_ERROR_CHECKS),
    Testcase("update_url_valid_3.json", Extension::INTERNAL,
             Extension::STRICT_ERROR_CHECKS),
    Testcase("update_url_valid_4.json", Extension::INTERNAL,
             Extension::STRICT_ERROR_CHECKS)
  };
  RunTestcases(testcases, arraysize(testcases),
               EXPECT_TYPE_SUCCESS);

  // Test some invalid update urls
  Testcase testcases2[] = {
    Testcase("update_url_invalid_1.json",
             errors::kInvalidUpdateURL,
             Extension::INTERNAL,
             Extension::STRICT_ERROR_CHECKS),
    Testcase("update_url_invalid_2.json",
             errors::kInvalidUpdateURL,
             Extension::INTERNAL,
             Extension::STRICT_ERROR_CHECKS),
    Testcase("update_url_invalid_3.json",
             errors::kInvalidUpdateURL,
             Extension::INTERNAL,
             Extension::STRICT_ERROR_CHECKS)
  };
  RunTestcases(testcases2, arraysize(testcases2),
               EXPECT_TYPE_ERROR);
}
