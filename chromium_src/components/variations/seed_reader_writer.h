/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_SEED_READER_WRITER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_SEED_READER_WRITER_H_

#include <string_view>

// Chromium 152 has ClearSessionCountry() but not SetSessionCountry(). Brave
// writes the X-Country header through SetSessionCountry so SeedFileTrial
// clients see mid-session country changes. Inject next to the unique
// ClearSessionCountry() declaration; do not hijack a token used in gtest
// macros or comments-as-code.
#define ClearSessionCountry() \
  ClearSessionCountry();      \
  void SetSessionCountry(std::string_view country_code)

#include <components/variations/seed_reader_writer.h>  // IWYU pragma: export

#undef ClearSessionCountry

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_SEED_READER_WRITER_H_
