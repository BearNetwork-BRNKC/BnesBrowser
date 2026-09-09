# BnesBrowser

**BnesBrowser** 是一個以 Chromium 為基礎的桌面瀏覽器，整合區塊鏈原生能力，包含 `bnes://` 協定、BNS（BNES Name Service）域名解析、原生加密錢包與後量子密碼學（PQC）支援。

## 特色

- **`bnes://` 協定** — 瀏覽器原生協定，直接存取 BNES 生態服務
- **BNS 域名解析** — 去中心化域名解析，整合於瀏覽器導覽層
- **原生加密錢包** — 內建於瀏覽器核心，無需擴充功能
- **PQC（後量子密碼學）** — 面向未來的加密能力
- **Chromium 引擎** — 完整的多程序架構、沙盒隔離與網路安全邊界

## 儲存庫說明

本儲存庫存放 BnesBrowser 的產品源碼與 Chromium 整合層，主要包含：

| 目錄 | 說明 |
| --- | --- |
| `bnes/` | BnesBrowser 核心自研邏輯（最高保護等級） |
| `browser/` `components/` `app/` | 瀏覽器功能、元件與應用層 |
| `chromium_src/` | 對 Chromium 源碼的覆寫 |
| `patches/` | Chromium 源碼補丁 |
| `resources/` | 產品資源與本地化 |
| `installer/` | 安裝封裝 |

版本對應：本版本基於 **Chromium 152.0.7977.64**。

## 從原始碼建置

BnesBrowser 的建置需要完整的 Chromium 建置環境（Windows / macOS / Linux），包括 `depot_tools`、GN、Ninja 與平台工具鏈。

建置前請先安裝 JavaScript 套件相依性：

```bash
pnpm install
```

詳細的建置環境設定、工具鏈版本與建置指令，請參閱 `docs/` 目錄與儲存庫內的建置腳本說明。

> **注意**：初次同步 Chromium 源碼需下載數十 GB 的資料，完整建置依硬體規格可能需要數小時。

## 關於程式碼簽章與 SmartScreen 警告

BnesBrowser 目前**未購買商業程式碼簽章憑證**，因此首次執行安裝程式時，Windows SmartScreen 可能顯示「Windows 已保護您的電腦」警告。請點選 **「更多資訊 → 仍要執行」** 繼續安裝。

這是我們的刻意選擇，而非疏忽：

> 商業 CA（憑證授權單位）體系的本質是**付費換取信任**——花錢就被標記為安全，不花錢就被標記為「不明的發行者」。這種中心化的信任評級機制，與 Web3 世界「信任源於密碼學驗證而非機構背書」的精神相悖。
>
> BnesBrowser 選擇以**密碼學方式**建立更新完整性（版本清單經離線保管的金鑰簽署、Releases 頁面公開 SHA-256 供自行驗證），而非向中心化機構購買信任評級。請務必在安裝前透過 `Get-FileHash`（PowerShell）比對 Releases 頁面公告的 SHA-256。

## 更新

BnesBrowser 會在偵測到新版本時於瀏覽器內顯示更新通知，並引導至本頁面的 [Releases](https://github.com/BearNetwork-BRNKC/BnesBrowser/releases) 下載最新版本。

## 參與貢獻

歡迎提交 Issue 與 Pull Request。提交前請參閱 [CONTRIBUTING.md](./CONTRIBUTING.md)。

## 安全性

如果你發現安全漏洞，請參閱 [SECURITY.md](./SECURITY.md) 的通報方式。**請勿直接以公開 Issue 回報安全漏洞。**

## 授權條款

請參閱 [LICENSE](./LICENSE)。
