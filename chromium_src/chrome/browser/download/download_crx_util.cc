/* Copyright (c) 2026 The BNES Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// BNES_GUARD wallet-crx-allow
// BNES Brave Overlay for download_crx_util.cc
//
// 目的：讓 BNES PQC 錢包 CRX 在拖放到 brave://extensions/ 時，
// 被視為可信任的擴充安裝（走安裝流程）而非普通下載。
//
// 問題根源：
//   IsTrustedExtensionDownload() 需要以下條件之一：
//     1. OffStoreInstallAllowedByPrefs()  — 需要 policy install_sources（一般無）
//     2. IsWebstoreUpdateUrl()            — BNES CRX 非 Web Store URL
//     3. IsWebstoreDomain()              — 同上
//   三個條件都不滿足，因此 CRX 被當作普通下載。
//
// 解決方案：
//   用 #define 技巧將上游函數重命名為 _Chromium 版本，
//   BNES 版本在 file:// scheme 的 .crx 下載時直接回傳 true，
//   其餘情況委託給上游邏輯。
//
//   安全保障：即使繞過此處的信任檢查，仍有：
//     (a) CrxVerifier overlay：僅 kBnesWalletExtensionId 可缺 publisher proof
//     (b) CrxInstaller::AllowInstall overlay：同一 ID 才放行 off-store
//     (c) InstallVerifier overlay：NeedsVerification / IsUnpackedLocation
//     (d) BraveExtensionProvider::MustRemainEnabled 防止被停用
//
// 上游同步策略：
//   若上游修改 IsTrustedExtensionDownload 函數簽名，
//   需同步更新此檔案中的 BNES 包裝函數簽名。

#include "base/files/file_path.h"
#include "components/download/public/common/download_item.h"
#include "extensions/browser/extension_util.h"

// 將上游 IsTrustedExtensionDownload 重命名為 _Chromium 供 BNES 包裝呼叫。
#define IsTrustedExtensionDownload IsTrustedExtensionDownload_Chromium

#include <chrome/browser/download/download_crx_util.cc>

#undef IsTrustedExtensionDownload

namespace download_crx_util {

bool IsTrustedExtensionDownload(Profile* profile,
                                const download::DownloadItem& item) {
  // BNES：對於從本機 file:// 拖放的 .crx 擴充功能，直接放行安裝流程。
  //
  // 判斷條件：
  //   1. IsExtensionDownload() — MIME type 必須是 application/x-chrome-extension
  //   2. URL scheme 為 file:// — 表示是本機拖放安裝，非網路下載
  //   3. 副檔名為 .crx — 雙重確認
  //
  // 此邏輯與 ExtensionManagement::IsOffstoreInstallAllowed 中的
  // `url.SchemeIsFile()` 豁免邏輯一致（即：file:// 不需要 referrer 驗證）。
  if (extensions::util::IsExtensionDownload(item)) {
    const GURL& url = item.GetURL();
    const GURL& original_url = item.GetOriginalUrl();

    // 本機檔案拖放：URL scheme 為 file://
    bool is_local_file = url.SchemeIsFile() || original_url.SchemeIsFile();

    if (is_local_file) {
      // 確認目標副檔名為 .crx
      const base::FilePath& target_path = item.GetTargetFilePath();
      if (!target_path.empty() &&
          target_path.MatchesExtension(FILE_PATH_LITERAL(".crx"))) {
        return true;
      }
      // 目標路徑可能尚未設定，從 URL path 推斷
      // Chromium 152: GURL::path() returns std::string_view; FilePath cannot
      // be constructed from it directly. FromUTF8Unsafe accepts string_view.
      base::FilePath url_path = base::FilePath::FromUTF8Unsafe(url.path());
      if (url_path.MatchesExtension(FILE_PATH_LITERAL(".crx"))) {
        return true;
      }
    }
  }

  // 其他情況委託給上游 Chromium 邏輯。
  return IsTrustedExtensionDownload_Chromium(profile, item);
}

}  // namespace download_crx_util
