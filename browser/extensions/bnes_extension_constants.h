/* Copyright (c) 2026 The BNES Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BNES_EXTENSION_CONSTANTS_H_
#define BRAVE_BROWSER_EXTENSIONS_BNES_EXTENSION_CONSTANTS_H_

#include <string_view>

// BNES_GUARD wallet-crx-allow

// BNES PQC 錢包的 Extension ID。
// 此 ID 由 metamask-extension/bnes-metamask.pem 私鑰決定，唯一且固定。
// 此常數是整個 BNES Brave Overlay 中唯一的 ID 真相來源。
// 若未來 PEM 金鑰更換，僅需修改此一處，並同步
// chromium_src/components/crx_file/crx_verifier.cc 裡的同值字串
// （該 overlay 位於 //components/crx_file，不能 include 本 header）。
//
// 上游合併時必須保留以下 BNES-owned / BNES 標記的放行點，缺一不可：
//   1. 本檔（ID 真相來源）
//   2. chromium_src/components/crx_file/crx_verifier.cc
//      — CRX3 publisher proof 豁免（拖放安裝 + 自動更新）
//   3. chromium_src/extensions/browser/crx_installer.{h,cc}
//      — off-store 安裝放行
//   4. chromium_src/extensions/browser/install_verifier.{h,cc}
//      — NeedsVerification / IsUnpackedLocation 豁免
//   5. chromium_src/chrome/browser/download/download_crx_util.cc
//      — 本機 file:// CRX 走安裝流程而非當普通下載
//   6. chromium_src/chrome/browser/resources/extensions/drop_overlay.ts
//      — brave://extensions/ 拖放 overlay 不依賴 Developer mode
//   7. browser/extensions/brave_extension_provider.cc MustRemainEnabled
inline constexpr char kBnesWalletExtensionId[] =
    "mjkhlgmnolenfmeiobklbfclkmbopinj";

inline constexpr bool IsBnesWalletExtensionId(std::string_view id) {
  return id == kBnesWalletExtensionId;
}

#endif  // BRAVE_BROWSER_EXTENSIONS_BNES_EXTENSION_CONSTANTS_H_
