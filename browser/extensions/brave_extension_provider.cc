/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BnesBrowser/browser/extensions/brave_extension_provider.h"

#include <string>

#include "base/notreached.h"
#include "BnesBrowser/browser/extensions/bnes_extension_constants.h"
#include "extensions/common/constants.h"

namespace extensions {

BraveExtensionProvider::BraveExtensionProvider() = default;

BraveExtensionProvider::~BraveExtensionProvider() = default;

std::string BraveExtensionProvider::GetDebugPolicyProviderName() const {
#if defined(NDEBUG)
  NOTREACHED();
#else
  return "Brave Extension Provider";
#endif
}

bool BraveExtensionProvider::MustRemainInstalled(const Extension* extension,
                                                  std::u16string* error) const {
  return extension->id() == brave_extension_id;
}

// BNES_GUARD wallet-crx-allow
// BNES: 對 PQC 錢包套用 MustRemainEnabled，防止 InstallVerifier
// 透過 MustRemainDisabled 強制將非商店 CRX 停用。
// 此為安裝後防線之一。CRX 檔頭 publisher proof 與 off-store 放行見
// bnes_extension_constants.h 列出的 overlay 清單。
bool BraveExtensionProvider::MustRemainEnabled(const Extension* extension,
                                               std::u16string* error) const {
  return extension->id() == kBnesWalletExtensionId;
}

}  // namespace extensions
