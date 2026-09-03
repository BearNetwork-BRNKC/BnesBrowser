/* Copyright (c) 2026 The BNES Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// BNES_GUARD wallet-crx-allow
// BNES: after CRX unpack, if the extension ID is the BNES wallet, treat the
// install as an off-store settings-page install. Combined with the
// crx_verifier overlay this lets drag-drop and auto-update work without a
// Chrome Web Store / Brave publisher proof.
//
// Upstream merge: this file is BNES-owned. Keep the ID check in sync with
// kBnesWalletExtensionId.

#include "extensions/browser/crx_installer.h"

#include <optional>

#include "BnesBrowser/browser/extensions/bnes_extension_constants.h"
#include "extensions/browser/install/crx_install_error.h"
#include "extensions/common/extension.h"

#define AllowInstall AllowInstall_ChromiumImpl
#include <extensions/browser/crx_installer.cc>
#undef AllowInstall

namespace extensions {

std::optional<CrxInstallError> CrxInstaller::AllowInstall(
    const Extension* extension) {
  if (extension && IsBnesWalletExtensionId(extension->id())) {
    off_store_install_allow_reason_ =
        OffStoreInstallAllowedFromSettingsPage;
  }
  return AllowInstall_ChromiumImpl(extension);
}

}  // namespace extensions
