// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_H_

#include "base/values.h"

namespace syncer {

// Whether a peer device supports being remotely told to delete itself.
enum class SelfDeleteSupport {
  kNotSupported,
  kSupported,
};

}  // namespace syncer

// rewrite/components/sync_device_info/device_info.h.yaml is supposed to patch
// these into Chromium. SafeBuild often skips apply_patches, so inject them
// here. DeepCopyForTesting() is unique; leftover `const;` completes ToValue().
#define DeepCopyForTesting()     \
  DeepCopyForTesting() const;    \
  DeviceInfo(DeviceInfo&&);      \
  std::string GetOSString() const; \
  std::string GetDeviceTypeString() const; \
  base::DictValue ToValue()

#include <components/sync_device_info/device_info.h>  // IWYU pragma: export

#undef DeepCopyForTesting

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_H_
