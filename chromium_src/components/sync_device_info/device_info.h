// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_H_

#include "base/values.h"

// NOTE: apply_patches was not applied to components/sync_device_info/device_info.h.
// The Chromium version in use already natively contains the following members
// that Brave's patch intended to add:
//   - DeviceInfo(DeviceInfo&&)
//   - SelfDeleteSupport self_delete_support() / set_self_delete_support()
//   - GetOSString() const
//   - GetDeviceTypeString() const
//   - base::DictValue ToValue() const
//   - SelfDeleteSupport self_delete_support_  (private field)
//
// The SelfDeleteSupport enum is also already declared in the Chromium header
// (used in constructor / accessor signatures below). We define it here in the
// syncer namespace so the overlay compiles before including the real header.
//
// The original macro-injection fallback (which re-declared all those members)
// caused "class member cannot be redeclared" errors because the Chromium
// upstream already includes them. The macro is now a no-op pass-through.

namespace syncer {

// Whether a peer device supports being remotely told to delete itself.
// Declared here so downstream callers that include this overlay get the type
// even if the real Chromium header hasn't been processed yet.
enum class SelfDeleteSupport {
  kNotSupported,
  kSupported,
};

}  // namespace syncer

// No macro injection needed: the Chromium header already declares all members.
// Keep the #define / #undef pair as a no-op so any third-party includes that
// rely on the macro being defined (and then undefined) don't break.
#define DeepCopyForTesting() DeepCopyForTesting() const

#include <components/sync_device_info/device_info.h>  // IWYU pragma: export

#undef DeepCopyForTesting

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_H_
