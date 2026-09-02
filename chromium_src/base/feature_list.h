/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_FEATURE_LIST_H_
#define BRAVE_CHROMIUM_SRC_BASE_FEATURE_LIST_H_

// Inject FeatureList::GetCompileTimeFeatureState() next to the existing
// public GetStateIfOverridden() declaration. The out-of-line definition lives
// in chromium_src/base/feature_list.cc and honors OVERRIDE_FEATURE_DEFAULT_STATES.
#define GetStateIfOverridden                                              \
  GetStateIfOverridden(const Feature& feature);                           \
  static FeatureState GetCompileTimeFeatureState(const Feature& feature); \
  static std::optional<bool> GetStateIfOverridden

#include <base/feature_list.h>  // IWYU pragma: export

#undef GetStateIfOverridden

#endif  // BRAVE_CHROMIUM_SRC_BASE_FEATURE_LIST_H_
