/* Copyright (c) 2026 The BNES Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_INSTALL_VERIFIER_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_INSTALL_VERIFIER_H_

// BNES_GUARD wallet-crx-allow
// BNES: wrap NeedsVerification so the wallet CRX skips Web Store origin
// checks. Restores the hook lost when install_verifier.cc moved from
// chrome/browser/extensions to extensions/browser.
#define NeedsVerification                                             \
  NeedsVerification(const Extension& extension,                       \
                    content::BrowserContext* context);                \
  static bool NeedsVerification_ChromiumImpl

#include <extensions/browser/install_verifier.h>  // IWYU pragma: export

#undef NeedsVerification

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_INSTALL_VERIFIER_H_
