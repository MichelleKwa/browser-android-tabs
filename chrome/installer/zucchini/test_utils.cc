// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/test_utils.h"

#include <ios>
#include <sstream>

#include "base/logging.h"

namespace zucchini {

std::vector<uint8_t> ParseHexString(const std::string& hex_string) {
  std::vector<uint8_t> ret;
  std::istringstream iss(hex_string);
  iss >> std::hex;
  uint32_t temp = 0;  // Cannot be uint8_t: istringstream treats this as char!
  while (iss >> temp) {
    CHECK_LE(temp, 0xFFU);
    ret.push_back(temp);
  }
  return ret;
}

}  // namespace zucchini
