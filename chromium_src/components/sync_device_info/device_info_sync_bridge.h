/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_SYNC_BRIDGE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_SYNC_BRIDGE_H_

// rewrite/components/sync_device_info/device_info_sync_bridge.h.yaml is
// supposed to patch these into Chromium. Inject next to the unique
// RefreshLocalDeviceInfoIfNeeded() declaration when apply_patches was skipped.
#define RefreshLocalDeviceInfoIfNeeded()                                      \
  RefreshLocalDeviceInfoIfNeeded();                                           \
  void DeleteDeviceInfo(const std::string& client_id,                         \
                        base::OnceClosure callback) override;                 \
  std::vector<DeviceInfo> GetAllBraveDeviceInfo() const override;             \
  void OnDeviceInfoDeleted(const std::string& client_id, const int attempt,   \
                           base::OnceClosure callback)

#include <components/sync_device_info/device_info_sync_bridge.h>  // IWYU pragma: export

#undef RefreshLocalDeviceInfoIfNeeded

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_SYNC_BRIDGE_H_
