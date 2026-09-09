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

## 參與貢獻

歡迎提交 Issue 與 Pull Request。提交前請參閱 [CONTRIBUTING.md](./CONTRIBUTING.md)。

## 安全性

如果你發現安全漏洞，請參閱 [SECURITY.md](./SECURITY.md) 的通報方式。**請勿直接以公開 Issue 回報安全漏洞。**

## 授權條款

請參閱 [LICENSE](./LICENSE)。
