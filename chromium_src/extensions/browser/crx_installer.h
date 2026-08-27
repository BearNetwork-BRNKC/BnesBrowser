/* Copyright (c) 2026 The BNES Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_CRX_INSTALLER_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_CRX_INSTALLER_H_

// BNES_GUARD wallet-crx-allow
// BNES: wrap AllowInstall so the wallet CRX can be installed off-store.
#define AllowInstall                                      \
  AllowInstall(const Extension* extension);               \
  std::optional<CrxInstallError> AllowInstall_ChromiumImpl

#include <extensions/browser/crx_installer.h>  // IWYU pragma: export

#undef AllowInstall

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_CRX_INSTALLER_H_
