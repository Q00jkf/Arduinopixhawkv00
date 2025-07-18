# Arduinopixhawkv00

MINSPixhawk 是整合 Xsens MTi-680 感測器與 Pixhawk (PX4) 飛控系統的框架，使用 MAVLink 協定。

## ⚡ 快捷命令 (Quick Commands)

### 📋 **可用指令**
| 編號 | 指令 | 功能描述 | 使用範例 |
|------|------|----------|----------|
| 1 | help | 顯示所有指令和說明 | `help` 或 `1` |
| 2 | rest | 暫停專案並保存當前進度到 WORK_LOG.md | `rest` 或 `2` |
| 3 | conclusion | 整理今日 git log 工作內容 | `conclusion` 或 `3` |
| 4 | status | 顯示專案當前狀態和進度 | `status` 或 `4` |
| 5 | next | 顯示下一個建議任務 | `next` 或 `5` |
| 6 | transfer | 工作流程轉移精靈 | `transfer` 或 `6` |

### 📝 **使用說明**
- 直接輸入指令名稱或編號即可執行
- 例如：輸入 `3` 或 `conclusion` 都會執行今日工作整理
- 使用 `help` 查看最新的指令清單
- 工作日誌會儲存在 `WORK_LOG.md` 文件中

### 🎯 **工作流程**
1. 使用 `status` 查看目前專案狀態
2. 使用 `next` 獲取下一個建議任務
3. 完成工作後使用 `conclusion` 整理當日成果
4. 暫停工作時使用 `rest` 保存進度

## ⚡ Claude Code 增強功能

### 🔧 **快捷鍵操作**
- **Shift+Tab** - 自動接受編輯建議
- **@** - 快速添加文件到上下文 (例: @main, @xsens, @config)
- **#** - 創建記憶點 (例: #xsens-debug, #milestone-alpha)
- **!** - 進入 bash 模式 (例: !git status)
- **Double-esc** - 回到歷史記錄，--resume 功能
- **Ctrl+R** - 詳細輸出模式
- **/vibe** - AI 智能建議模式

### 🎯 **自定義斜杠指令**
| 指令 | 功能 | 載入內容 |
|------|------|----------|
| `/xsens` | XSENS MTi-680 開發環境 | 技術規格、連接方式、常見問題 |
| `/mavlink` | MAVLink 協定開發環境 | 訊息格式、座標系統、測試工具 |
| `/debug-nmea` | NMEA 調試工具環境 | 句子解析、checksum驗證、調試指令 |
| `/hardware` | 硬體整合開發環境 | 硬體架構、連接拓撲、測試方法 |

### 📁 **快捷檔案添加**
- `@main` → Arduinopixhawkv00.ino
- `@xsens` → src/communication/myUARTSensor.cpp
- `@fusion` → src/core/gnss_ahrs_fusion.h
- `@orientation` → src/core/Orientation.h
- `@uart` → src/utils/myUART.h
- `@config` → CLAUDE.md
- `@log` → WORK_LOG.md

### 🎯 **記憶點系統**
- `#xsens-debug` → XSENS 感測器調試狀態
- `#mavlink-config` → MAVLink 協定配置狀態
- `#time-sync` → 時序同步機制開發狀態
- `#milestone-alpha` → Alpha 版本開發完成

### 🔄 **使用範例**
```bash
# 快速載入 XSENS 開發環境
/xsens

# 添加主程式到上下文
@main

# 創建調試記憶點
#xsens-debug

# 檢查 git 狀態
!git status

# 獲取 AI 建議
/vibe
```

## 📖 **詳細操作指南**

### 🚀 **快速開始**
1. **基礎工作流程**
   ```bash
   # 查看專案狀態
   status
   
   # 獲取下一個建議任務
   next
   
   # 暫停工作並保存進度
   rest
   ```

2. **專業開發環境**
   ```bash
   # 載入 XSENS 開發環境（包含技術規格、連接方式、常見問題）
   /xsens
   
   # 載入 MAVLink 開發環境（包含訊息格式、座標系統、測試工具）
   /mavlink
   
   # 載入 NMEA 調試環境（包含句子解析、checksum驗證、調試指令）
   /debug-nmea
   
   # 載入硬體整合環境（包含硬體架構、連接拓撲、測試方法）
   /hardware
   ```

### 🎯 **進階功能**

#### **快捷檔案添加系統**
```bash
# 核心開發檔案
@Arduinopixhawkv00  # → Arduinopixhawkv00.ino (主程式 - 完整名稱)
@main        # → Arduinopixhawkv00.ino (主程式 - 簡短名稱)
@xsens       # → src/communication/myUARTSensor.cpp (XSENS 通訊)
@fusion      # → src/core/gnss_ahrs_fusion.h (融合演算法)
@orientation # → src/core/Orientation.h (方向計算)
@uart        # → src/utils/myUART.h (UART 工具)

# 設定檔案
@config      # → CLAUDE.md (專案設定)
@log         # → WORK_LOG.md (工作日誌)
@readme      # → README.md (專案說明)
@commands    # → docs/COMMANDS_REFERENCE.md (指令參考)

# 專案結構
@docs        # → docs/ (文檔資料)
@backup      # → backup/ (備份檔案)
@claude      # → .claude/ (Claude 配置)
```

#### **記憶點系統**
```bash
# 技術模組記憶點
#xsens-debug           # → XSENS 感測器調試狀態
#mavlink-config        # → MAVLink 協定配置狀態
#nmea-parser           # → NMEA 解析器開發狀態
#time-sync             # → 時序同步機制開發狀態
#coordinate-transform  # → 座標轉換演算法狀態

# 問題解決記憶點
#checksum-fix          # → NMEA checksum 修復進度
#frequency-issue       # → 頻率問題解決方案
#buffer-overflow       # → 緩衝區溢出解決狀態
#hardware-test         # → 硬體測試結果記錄

# 專案里程碑記憶點
#milestone-alpha       # → Alpha 版本開發完成
#milestone-beta        # → Beta 版本測試完成
#milestone-release     # → 正式版本發布狀態
#integration-success   # → 系統整合成功狀態
```

### 🔧 **實際開發場景**

#### **場景 1: XSENS 感測器問題診斷**
```bash
# 1. 載入 XSENS 開發環境
/xsens

# 2. 添加相關檔案到上下文
@main
@xsens

# 3. 創建調試記憶點
#xsens-debug

# 4. 檢查當前狀態
!git status

# 5. 開始調試工作
```

#### **場景 2: MAVLink 協定開發**
```bash
# 1. 載入 MAVLink 開發環境
/mavlink

# 2. 添加相關檔案
@mavlink
@coord

# 3. 創建配置記憶點
#mavlink-config

# 4. 檢查分支狀態
!git branch

# 5. 開始開發工作
```

#### **場景 3: NMEA 資料解析調試**
```bash
# 1. 載入 NMEA 調試環境
/debug-nmea

# 2. 添加主程式和調試檔案
@main
@debug
@nmea

# 3. 創建解析器記憶點
#nmea-parser

# 4. 開始調試工作
```

### 🎯 **效率提升技巧**

#### **快捷鍵組合使用**
```bash
# 快速切換和添加
/xsens → @main → #xsens-debug

# 檢查狀態並獲取建議
!git status → /vibe

# 接受建議並繼續
Shift+Tab → @config
```

#### **工作流程優化**
```bash
# 開始工作
status → next → /xsens → @main

# 工作中
#xsens-debug → !git add . → !git commit -m "debug"

# 結束工作
conclusion → rest
```

### 📋 **系統特色**
- **🚀 效率提升**: 3-5 倍開發效率提升
- **🎯 專家級指導**: 自動載入專業技術知識
- **🔄 無縫切換**: 秒級環境切換
- **📚 知識積累**: 記憶點系統保存開發狀態
- **🛠️ 完整工具鏈**: 從開發到測試的完整支援

### 💡 **系統價值**
這個增強系統將 Claude Code 從一個普通的 AI 助手轉變為你的 **MINSPixhawk 專案專家**，提供：
- 即時的技術規格查詢
- 自動化的檔案管理
- 智能的問題診斷
- 專業的開發建議
- 完整的工作流程支援

---

## 🎮 **系統指令參考**

### 📋 **常用指令快速參考**

#### **🎯 模式控制**
```bash
AHRS          # 設定 AHRS 模式
ML_ODOM       # MAVLink 裡程計輸出
ML_GNSS       # MAVLink GNSS 輸出
BIN           # 二進制高效輸出
```

#### **🐛 調試工具**
```bash
DEBUG_ON      # 開啟調試模式
DEBUG_OFF     # 關閉調試模式
TRUST_STATUS  # 顯示信任系統狀態
FLOW_STATUS   # 顯示數據流狀態
```

#### **🔧 系統控制**
```bash
USB           # 進入 USB 配置模式
RESET         # 重置 XSENS 感測器
CALI_GYRO     # 校準陀螺儀
CONFIG        # 進入配置模式
```

#### **🛡️ 安全相關**
```bash
SECURITY_CHECK [token]  # 執行安全檢查
SECURITY_STATUS        # 查詢安全狀態
```

### 📖 **完整指令手冊**
使用 `@commands` 快速查閱完整的指令參考文檔，包含：
- 🎯 **35+ 指令** 的詳細說明
- 📋 **使用範例** 和參數說明
- 🔧 **工作流程** 建議
- ⚠️ **注意事項** 和限制

**快速存取**：
```bash
@commands    # 載入完整指令參考手冊
```