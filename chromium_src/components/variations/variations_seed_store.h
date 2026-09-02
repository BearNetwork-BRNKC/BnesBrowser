/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_SEED_STORE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_SEED_STORE_H_

#include <string_view>

// Chromium 152 has no VariationsSeedStore::SetSessionCountry(). The overlay
// .cc and variations_service.cc (X-Country on HTTP 304) need it. Inject next
// to the unique GetLatestCountry() declaration.
#define GetLatestCountry() \
  GetLatestCountry();      \
  void SetSessionCountry(std::string_view country_code)

#include <components/variations/variations_seed_store.h>  // IWYU pragma: export

#undef GetLatestCountry

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_SEED_STORE_H_
