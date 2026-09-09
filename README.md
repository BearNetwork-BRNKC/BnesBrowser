# BnesBrowser

**BnesBrowser** 是一個以 Chromium 為基礎的桌面瀏覽器，整合區塊鏈原生能力，包含 `bnes://` 協定、BNS（BNES Name Service）域名解析、原生加密錢包與後量子密碼學（PQC）支援。

## 特色

- **`bnes://` 協定** — 瀏覽器原生協定，直接存取 BNES 生態服務
- **BNS 域名解析** — 去中心化域名解析，整合於瀏覽器導覽層
- **原生加密錢包** — 內建於瀏覽器核心，無需擴充功能
- **PQC（後量子密碼學）** — 面向未來的加密能力，並延伸至更新完整性驗證（見下方說明）
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
> BnesBrowser 選擇以**後量子密碼學（PQC）**建立更新完整性：版本清單由離線保管的金鑰以 **ML-DSA-65（NIST FIPS 204）** 簽署，驗證公鑰直接內嵌於瀏覽器程式碼——信任根是一把離線金鑰與可公開驗證的密碼學，而不是一家簽證機構的收據。

**安裝前請自行驗證檔案完整性**（PowerShell）：

```powershell
Get-FileHash .\BnesBrowser_setup.exe -Algorithm SHA256
```

並比對 Releases 頁面公告的 SHA-256 值。

## 更新

BnesBrowser 的更新機制分兩個階段建置：

- **階段一（已規劃）**：瀏覽器內建更新檢查，偵測到新版本時顯示通知，引導至 [Releases](https://github.com/BearNetwork-BRNKC/BnesBrowser/releases) 頁面下載
- **階段二（規劃中）**：背景自動更新——新版安裝包下載後，先以內嵌公鑰完成 **ML-DSA-65 簽章驗證** 與 SHA-256 比對，全部通過才執行靜默安裝；驗證失敗即放棄，絕不安裝未經驗證的內容

無論哪個階段，更新管道的信任根都是密碼學（內嵌公鑰 + 離線私鑰），不是任何中心化機構。

## 參與貢獻

歡迎提交 Issue 與 Pull Request。提交前請參閱 [CONTRIBUTING.md](./CONTRIBUTING.md)。

## 安全性

如果你發現安全漏洞，請參閱 [SECURITY.md](./SECURITY.md) 的通報方式。**請勿直接以公開 Issue 回報安全漏洞。**

## 授權條款

請參閱 [LICENSE](./LICENSE)。
