/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * No-op fallback. The Brave method additions
 * (DeleteDeviceInfo / GetAllBraveDeviceInfo / OnDeviceInfoDeleted) are
 * already present in the upstream Chromium header at
 * src/components/sync_device_info/device_info_sync_bridge.h
 * (apply_patches applied rewrite/components/sync_device_info/
 * device_info_sync_bridge.h.yaml during build-graph evaluation).
 * Including the Chromium header here is sufficient; injecting via a
 * preprocessor macro would create duplicate class members.
 */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_SYNC_BRIDGE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_SYNC_BRIDGE_H_

#include <components/sync_device_info/device_info_sync_bridge.h>

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_SYNC_BRIDGE_H_