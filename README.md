![BnesBrowser](./docs/images/brave.svg)

# BnesBrowser Core

BnesBrowser Core 是一組用於自訂 Chromium 的變更、API 和腳本，
目的是打造 BnesBrowser 。 另外請參閱
https://github.com/BearNetwork-BRNKC/BnesBrowser ，該倉庫僅存放議題、發佈版本以及
wiki。

## 概述

本倉庫存放建置 BnesBrowser 桌面瀏覽器所需的工具，支援所有平台。
特別是會從 `package.json` 和 `src/brave/DEPS` 中定義的專案取得與同步程式碼：

- [Chromium](https://chromium.googlesource.com/chromium/src.git)
  - 透過 `depot_tools` 取得程式碼。
  - 設定 Chromium 的分支（例如：65.0.3325.181）。
- [brave-core](https://github.com/brave/brave-core)
  - 掛載於 `src/brave`。
  - 維護對第三方 Chromium 程式碼的修補程式。
- [adblock-rust](https://github.com/brave/adblock-rust)
  - 實作 Brave 的廣告封鎖引擎。
  - 透過
    [brave/adblock-rust-ffi](https://github.com/brave/brave-core/tree/master/components/adblock_rust_ffi)
    連結。

## 資源

- [文件與指南](https://github.com/brave/brave-core/blob/master/docs/README.md)
- [議題追蹤](https://github.com/brave/brave-browser/issues)
- [發佈版本](https://github.com/brave/brave-browser/releases)
- [Wiki](https://github.com/brave/brave-browser/wiki)

## 下載

您可以[造覽我們的網站](https://brave.com/download)取得最新的穩定
發佈版本。

## 參與貢獻

請參閱[參與指南](./CONTRIBUTING.md)。

我們的[Wiki](https://github.com/brave/brave-browser/wiki)也有許多有用的
技術資訊，特別是關於設定開發環境的說明。

## 安全性政策

請參閱[安全性政策](./SECURITY.md)。

## 社群

如果您想更深入參與 Brave，請[加入 Q&A 社群](https://community.brave.app/)。
您可以
[尋求協助](https://community.brave.app/c/support-and-troubleshooting)，
[討論您希望看到的功能](https://community.brave.app/c/brave-feature-requests)，
以及更多其他活動。 我們非常歡迎您的協助，讓我們能持續改進 Brave。

您也可以在 Brave Software 的
Slack 上的 [`community-guest`](https://bravesoftware.slack.com) 頻道中提問和互動。

請透過 https://explore.transifex.com/brave/brave_en/ 提交翻譯，
協助我們將 Brave 翻譯成您的語言。

在 X 上追蹤 [@brave](https://x.com/brave) 以取得重要新聞和公告。

## 安裝先決條件

請根據您的平台遵循相應的指示：

- [Android](https://github.com/brave/brave-browser/wiki/Android-Development-Environment)
- [Linux](https://github.com/brave/brave-browser/wiki/Linux-Development-Environment)
- [iOS](https://github.com/brave/brave-browser/wiki/iOS-Development-Environment)
- [macOS](https://github.com/brave/brave-browser/wiki/macOS-Development-Environment)
- [Windows](https://github.com/brave/brave-browser/wiki/Windows-Development-Environment)

## 複製並初始化

安裝好先決條件後，您可以取得程式碼並初始化建置環境。

**複製倉庫。** `BnesBrowser` 必須被複製到現有專案資料夾中的 `./src/brave` 目錄下：

```bash
git clone https://github.com/BearNetwork-BRNKC/BnesBrowser.git path-to-your-project-folder/src/brave
cd path-to-your-project-folder/src/brave
```

**初始化建置環境。** 此步驟將下載 Chromium
原始碼，其歷史記錄非常龐大（數十 GB 的資料）。 根據網路速度不同，這可能需要非常長的時間才能完成。

```bash
# 大多數建置：
pnpm run init

# Android 建置（將 `arm` 替換為您想要建置的 CPU 類型）：
pnpm run init --target_os=android --target_arch=arm

# iOS 建置：
pnpm run init --target_os=ios
```

其他建置所需的配置請參閱
https://github.com/brave/brave-browser/wiki/Build-configuration

內部開發人員可以在
https://github.com/brave/internal/wiki/Build-configuration
找到更多資訊。

## 建置 Brave

預設建置類型為元件建置（component build）。

```
# 開始元件建置編譯
pnpm run build
```

若要進行發佈建置：

```
# 開始發佈編譯
pnpm run build Release
```

基於 brave-core 的 Android 建置應使用
`pnpm run build --target_os=android --target_arch=arm`

基於 brave-core 的 iOS 建置應使用位於
`ios/brave-ios/App` 的 Xcode 專案。 您可以直接開啟該專案，
或執行 `pnpm run ios_bootstrap --open_xcodeproj` 讓它自動在 Xcode 中被打開。
有關 iOS 建置的更多資訊，請參閱
[iOS 開發者環境](https://github.com/brave/brave-browser/wiki/iOS-Development-Environment#Building)。

### 建置組態

使用 `pnpm run build Release` 進行發佈建置可能會非常緩慢，
並使用大量 RAM，特別是在使用 Gold LLVM 外掛程式的 Linux 上。

若要執行靜態連結建置（建置時間較長，但啟動速度較快）：

```bash
pnpm run build Static
```

若要執行偵錯建置（具有 is_debug=true 的元件建置）：

```bash
pnpm run build Debug
```

注意：建置將需要一段時間。 根據您的處理器和
記憶體，可能需要幾個小時。

## 執行 Brave

若要啟動建置：

`pnpm start [Release|Component|Static|Debug]`

## 更新 Brave

`pnpm run sync [--force] [--init] [--create] [brave_core_ref]`

**此指令會嘗試將您在 brave-core 中的本機變更存入暫存，但在執行前提交本機變更會更安全**

`pnpm run sync` 會（根據以下旗標）執行以下操作：

1. 📥 將子專案（chromium、brave-core）更新至 git 參考（例如標籤或分支）的最新提交
2. 🤕 套用修補程式
3. 🔄 更新 gclient DEPS 相依性
4. ⏩ 執行鉤子

| 旗標                           | 描述                                                                                                                                                                                                                                                                                                                                                                 |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `[無旗標]`                     | 在需要時更新 chromium 並重新套用修補程式。 如果 chromium 版本沒有變更，則只會重新套用已變更的修補程式。 **僅在本次指令執行期間有任何專案需要更新時**，才會更新子相依性。<br> **如果您希望指令自動管理更新狀態，而不是手動拉取或切換分支，請使用此選項。** |
| `--force`                      | 將 _Chromium_ 和 _brave-core_ 都更新至目前 brave-core 分支和 `brave-core/package.json` 中指定的 _Chromium_ 參考（例如 `master` 或 `74.0.0.103`）的最新遠端提交。 將重新套用所有修補程式。 將強制更新所有子相依性。<br> **如果您遇到問題並希望將分支強制恢復到已知狀態，請使用此選項。** |
| `--init`                       | 強制將 _Chromium_ 和 _brave-core_ 更新至 `brave-core/package.json` 中指定的版本，並強制更新所有相依倉庫 - 與 `pnpm run init` 相同                                                                                                                                                                                                                                 |
| `--sync_chromium (true/false)` | 在適用時強制或略過 chromium 版本更新。 如果您希望在未準備好承受 chromium 更新可能導致的大量建置時間時避免小版本更新，此選項非常有用。 將會輸出有關目前程式碼狀態預期不同 chromium 版本的警告。 這可能導致您的建置失敗。                                                                      |
| `-D, --delete_unused_deps`     | 將從工作複本中刪除自上次同步以來已移除的任何相依性。 模擬 `gclient sync -D`。                                                                                                                                                                                                                                                                                      |

執行 `pnpm run sync brave_core_ref` 以簽出指定的 _brave-core_ 參考
並更新所有相依倉庫，包括 chromium（如需要）。

## 情境

#### 建立新分支：

```bash
> cd src/brave
src/brave> git checkout -b branch_name
```

#### 簽出現有分支或標籤：

```bash
src/brave> git fetch origin
src/brave> git checkout [-b] branch_name
src/brave> pnpm run sync
...Updating 2 patches...
...Updating child dependencies...
...Running hooks...
```

#### 將目前分支更新至最新遠端版本：

```bash
src/brave> git pull
src/brave> pnpm run sync
...Updating 2 patches...
...Updating child dependencies...
...Running hooks...
```

#### 透過 `init` 重設至最新的 brave-core master（將導致較長的建置時間，並將移除 brave-core 工作目錄中所有未提交的變更）：

```bash
src/brave> git checkout master
src/brave> git pull
src/brave> pnpm run sync --init
```

#### 當您知道 DEPS 沒有變更，但 .patch 檔案有變更時（在建置前執行小型同步的最快嘗試）：

```bash
src/brave> git checkout featureB
src/brave> git pull
src/brave> pnpm run apply_patches
...Applying 2 patches...
```

## 啟用第三方 API

1. **Google Safe Browsing**：從
    https://console.developers.google.com/ 取得已啟用 SafeBrowsing API 的金鑰。
    根據 https://www.chromium.org/developers/how-tos/api-keys 的說明，
    使用您的金鑰更新 `GOOGLE_API_KEY` 環境變數以啟用 Google
    SafeBrowsing。

## 開發

- [來自 Chromium 的安全性規則](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/security/rules.md)
- [IPC 審查指南](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/ipc-reviews.md)
   （特別是
   [此參考文件](https://docs.google.com/document/d/1Kw4aTuISF7csHnjOpDJGc7JYIjlvOAKRprCTBVWw_E4/edit#heading=h.84bpc1e9z1bg)）
- [Brave 內部安全性指南](https://github.com/brave/internal/wiki/Pull-request-security-audit-checklist)
   （僅供員工使用）
- [Rust 使用說明](https://github.com/brave/brave-core/blob/master/docs/rust.md)

## 疑難排解

有關常見問題的解決方案，請參閱
[疑難排解](https://github.com/brave/brave-browser/wiki/Troubleshooting)。
