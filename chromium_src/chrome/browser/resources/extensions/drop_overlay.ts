// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// BNES_GUARD wallet-crx-allow
// BNES: always accept CRX/ZIP drops on brave://extensions/, even when
// Developer mode is off. Upstream binds dragEnabled to inDevMode, which
// makes the drop fall through as a file download and then fail with
// CRX_REQUIRED_PROOF_MISSING.
//
// Upstream merge: this file is BNES-owned. Do not put it under
// brave/chromium_src/... — chromium_src overlays live at the repo-root
// chromium_src/ path.

import { ExtensionsDropOverlayElement } from './drop_overlay-chromium.js'

const originalWillUpdate = ExtensionsDropOverlayElement.prototype.willUpdate

ExtensionsDropOverlayElement.prototype.willUpdate = function (
  this: ExtensionsDropOverlayElement,
  changedProperties: unknown,
) {
  originalWillUpdate.call(this, changedProperties as never)
  const self = this as unknown as {
    dragWrapperHandler_?: {dragEnabled: boolean}
  }
  if (self.dragWrapperHandler_) {
    self.dragWrapperHandler_.dragEnabled = true
  }
}

export * from './drop_overlay-chromium.js'
