/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_
#define BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_

#include "base/feature_list.h"

namespace base {

class FeatureList;

namespace internal {

class BASE_EXPORT FeatureDefaultStateOverrider {
 public:
  using FeatureOverrideInfo =
      std::pair<std::reference_wrapper<const Feature>, FeatureState>;

  FeatureDefaultStateOverrider(
      std::initializer_list<FeatureOverrideInfo> overrides);
};

}  // namespace internal
}  // namespace base

// Feature override uses global constructors, we disable `global-constructors`
// warning inside this macro to instantiate the overrider without warnings.
// clang-format off
#define OVERRIDE_FEATURE_DEFAULT_STATES(...)                    \
  _Pragma("clang diagnostic push")                              \
  _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") \
  static const ::base::internal::FeatureDefaultStateOverrider   \
      g_feature_default_state_overrider __VA_ARGS__;            \
  _Pragma("clang diagnostic pop")                               \
  static_assert(true, "") /* for a semicolon requirement */
// clang-format on

#endif  // BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_