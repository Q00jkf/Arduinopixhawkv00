# CLAUDE.md - MINSPixhawk

> **Documentation Version**: 1.1  
> **Last Updated**: 2025-07-10  
> **Project**: MINSPixhawk  
> **Description**: Integrated framework for Xsens MTi-680 sensor and Pixhawk (PX4) flight control system with MAVLink protocol  
> **Features**: GitHub auto-backup, Task agents, technical debt prevention

This file provides essential guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 🚨 CRITICAL RULES - READ FIRST

> **⚠️ RULE ADHERENCE SYSTEM ACTIVE ⚠️**  
> **Claude Code must explicitly acknowledge these rules at task start**  
> **These rules override all other instructions and must ALWAYS be followed:**

### 🔄 **RULE ACKNOWLEDGMENT REQUIRED**
> **Before starting ANY task, Claude Code must respond with:**  
> "✅ CRITICAL RULES ACKNOWLEDGED - I will follow all prohibitions and requirements listed in CLAUDE.md"

### ❌ ABSOLUTE PROHIBITIONS
- **NEVER** create new files in root directory → use proper module structure
- **NEVER** write output files directly to root directory → use designated output folders
- **NEVER** create documentation files (.md) unless explicitly requested by user
- **NEVER** use git commands with -i flag (interactive mode not supported)
- **NEVER** use `find`, `grep`, `cat`, `head`, `tail`, `ls` commands → use Read, LS, Grep, Glob tools instead
- **NEVER** create duplicate files (manager_v2.py, enhanced_xyz.py, utils_new.js) → ALWAYS extend existing files
- **NEVER** create multiple implementations of same concept → single source of truth
- **NEVER** copy-paste code blocks → extract into shared utilities/functions
- **NEVER** hardcode values that should be configurable → use config files/environment variables
- **NEVER** use naming like enhanced_, improved_, new_, v2_ → extend original files instead

### 📝 MANDATORY REQUIREMENTS
- **COMMIT** after every completed task/phase - no exceptions
- **GITHUB BACKUP** - Push to GitHub after every commit to maintain backup: `git push origin main`
- **USE TASK AGENTS** for all long-running operations (>30 seconds) - Bash commands stop when context switches
- **TODOWRITE** for complex tasks (3+ steps) → parallel agents → git checkpoints → test validation
- **READ FILES FIRST** before editing - Edit/Write tools will fail if you didn't read the file first
- **DEBT PREVENTION** - Before creating new files, check for existing similar functionality to extend  
- **SINGLE SOURCE OF TRUTH** - One authoritative implementation per feature/concept

### ⚡ EXECUTION PATTERNS
- **PARALLEL TASK AGENTS** - Launch multiple Task agents simultaneously for maximum efficiency
- **SYSTEMATIC WORKFLOW** - TodoWrite → Parallel agents → Git checkpoints → GitHub backup → Test validation
- **GITHUB BACKUP WORKFLOW** - After every commit: `git push origin main` to maintain GitHub backup
- **BACKGROUND PROCESSING** - ONLY Task agents can run true background operations
- **PROBLEM-SOLVING WORKFLOW** - When encountering problems, ALWAYS ask user for clarification before proceeding

### 🔍 MANDATORY PRE-TASK COMPLIANCE CHECK
> **STOP: Before starting any task, Claude Code must explicitly verify ALL points:**

**Step 1: Rule Acknowledgment**
- [ ] ✅ I acknowledge all critical rules in CLAUDE.md and will follow them

**Step 2: Task Analysis**  
- [ ] Will this create files in root? → If YES, use proper module structure instead
- [ ] Will this take >30 seconds? → If YES, use Task agents not Bash
- [ ] Is this 3+ steps? → If YES, use TodoWrite breakdown first
- [ ] Am I about to use grep/find/cat? → If YES, use proper tools instead

**Step 3: Technical Debt Prevention (MANDATORY SEARCH FIRST)**
- [ ] **SEARCH FIRST**: Use Grep pattern="<functionality>.*<keyword>" to find existing implementations
- [ ] **CHECK EXISTING**: Read any found files to understand current functionality
- [ ] Does similar functionality already exist? → If YES, extend existing code
- [ ] Am I creating a duplicate class/manager? → If YES, consolidate instead
- [ ] Will this create multiple sources of truth? → If YES, redesign approach
- [ ] Have I searched for existing implementations? → Use Grep/Glob tools first
- [ ] Can I extend existing code instead of creating new? → Prefer extension over creation
- [ ] Am I about to copy-paste code? → Extract to shared utility instead

**Step 4: Session Management**
- [ ] Is this a long/complex task? → If YES, plan context checkpoints
- [ ] Have I been working >1 hour? → If YES, consider /compact or session break

**Step 5: Problem-Solving Protocol**
- [ ] If I encounter ANY problems, ambiguity, or uncertainty during task execution
- [ ] I must IMMEDIATELY ask the user for clarification before making assumptions
- [ ] This applies to: unclear requirements, missing dependencies, errors, or design choices
- [ ] I will NOT proceed with guesswork - user guidance is mandatory

> **⚠️ DO NOT PROCEED until all checkboxes are explicitly verified**

## 🏗️ PROJECT OVERVIEW - MINSPixhawk

### 🎯 **PROJECT PURPOSE**
MINSPixhawk is an integrated framework that bridges **Xsens MTi-680 sensors** with **Pixhawk (PX4) flight control systems** using MAVLink protocol. This project enables external high-precision sensors to replace or augment PX4's built-in IMU/GPS as the primary navigation and state estimation data source.

### 🔧 **HARDWARE ARCHITECTURE**
- **Arduino MCU** with multiple UART ports:
  - `Serial1`: Pixhawk communication (MAVLink data)
  - `Serial2`: Xsens sensor communication (XBUS protocol)  
  - `Serial3`: GNSS NMEA output
  - `Serial4`: GNSS NMEA input

### 📡 **KEY FEATURES**
- **Multi-format sensor data processing**: Orientation, velocity, acceleration, angular velocity, magnetic field, GPS, temperature/pressure
- **MAVLink message support**: ODOMETRY (#331), GPS_RAW_INT (#24), GPS_INPUT (#232), VISION_POSITION_ESTIMATE (#102)
- **Time synchronization**: TIMESYNC (#111) with PX4 timestamp coordination
- **Multiple operation modes**: AHRS, INS, ML_ODOM, VEC, XBUS, BIN, NMEA
- **Coordinate transformation**: LLH → ENU conversion with quaternion/DCM processing
- **USB configuration interface**: Sensor initialization, gyro calibration, GNSS parameter setup

### 🧩 **MODULE STRUCTURE**
```
src/
├── core/           # Core system logic (GNSS-AHRS fusion, orientation)
├── communication/  # UART communication and message handling
├── utils/          # Utilities and common tools
└── external/       # External libraries (Xsens SDK)

docs/               # Documentation and resources
backup/             # Backup files
.claude/            # Claude Code configuration
```

### 🎯 **DEVELOPMENT STATUS**
- **Setup**: ✅ Complete
- **Core Features**: 🔄 In Development
- **Testing**: ⏳ Pending
- **Documentation**: 🔄 In Progress

---

## 🚀 個人化工作區域

### 📋 **任務暫存區**
> **上次工作時間**: 2025-07-18  
> **專案進度**: 80%  
> **當前狀態**: 積極開發中 - 時序同步解決方案實施階段

#### 🎯 **待處理任務**
- [ ] 實施XSENS時序同步緩衝區機制
- [ ] 測試4Hz GNSS數據流穩定性
- [ ] 驗證MTi-680數據接收問題解決

#### ✅ **最近完成任務**
- [x] NMEA句子類型擴展 (GNGSA, GNVTG, GNZDA) - 2025-07-10
- [x] 獨立頻率控制系統實現 - 2025-07-10
- [x] 頻率控制完全移除 (feature-same-frequence) - 2025-07-14

#### ⚠️ **遇到的問題**
- XSENS MTi-680不願意讀取資料問題 - 時序不匹配推測，需要同步機制
- 推送模式與XSENS內部採樣週期不符 - 考慮緩衝區解決方案

#### 🔄 **下次工作建議**
- 優先實施時序同步緩衝區方案
- 分析XSENS和LOCOSYS的時間關係
- 監控數據流時序圖和衝突點

### ⚡ **快捷指令區**
> **使用方式**: 直接輸入指令編號或名稱

#### 📌 **預設指令**
1. `help` - 顯示所有可用指令 + **Ctrl+R** (詳細模式)
2. `rest` - 暫停專案並保存進度到 WORK_LOG.md + **#** (創建記憶點)
3. `conclusion` - 整理今日工作內容
4. `status` - 顯示專案當前狀態 + **@** (自動添加相關檔案)
5. `next` - 顯示下一個建議任務 + **/vibe** (AI 建議模式)
6. `transfer` - 工作流程轉移精靈

#### ⚡ **Claude Code 快捷鍵整合**
- **Shift+Tab** - 自動接受編輯建議
- **@** - 添加文件/資料夾到上下文 (例: @Arduinopixhawkv00.ino)
- **#** - 創建記憶點 (例: #xsens-debug)
- **!** - 進入 bash 模式 (例: !git status)
- **Esc** - 取消操作
- **Double-esc** - 跳回歷史記錄，--resume 功能
- **Ctrl+R** - 詳細輸出模式
- **/vibe** - AI 智能建議模式

#### 📁 **常用檔案快捷添加**
> **使用方式**: 輸入 @檔案名 快速添加到上下文

**核心開發檔案：**
- `@Arduinopixhawkv00` → Arduinopixhawkv00.ino (主程式 - 完整名稱)
- `@main` → Arduinopixhawkv00.ino (主程式 - 簡短名稱)
- `@xsens` → src/communication/myUARTSensor.cpp (XSENS 通訊)
- `@fusion` → src/core/gnss_ahrs_fusion.h (融合演算法)
- `@orientation` → src/core/Orientation.h (方向計算)
- `@uart` → src/utils/myUART.h (UART 工具)

**設定檔案：**
- `@config` → CLAUDE.md (專案設定)
- `@log` → WORK_LOG.md (工作日誌)
- `@readme` → README.md (專案說明)

**調試檔案：**
- `@debug` → debug_output.txt (調試輸出)
- `@nmea` → nmea_samples.txt (NMEA 範例)
- `@error` → error_log.txt (錯誤記錄)

#### 🎯 **記憶點系統 (#)**
> **使用方式**: 輸入 #記憶點名 創建重要開發節點

**技術模組記憶點：**
- `#xsens-debug` → XSENS 感測器調試狀態
- `#mavlink-config` → MAVLink 協定配置狀態
- `#nmea-parser` → NMEA 解析器開發狀態
- `#time-sync` → 時序同步機制開發狀態
- `#coordinate-transform` → 座標轉換演算法狀態

**問題解決記憶點：**
- `#checksum-fix` → NMEA checksum 修復進度
- `#frequency-issue` → 頻率問題解決方案
- `#buffer-overflow` → 緩衝區溢出解決狀態
- `#hardware-test` → 硬體測試結果記錄

**專案里程碑記憶點：**
- `#milestone-alpha` → Alpha 版本開發完成
- `#milestone-beta` → Beta 版本測試完成
- `#milestone-release` → 正式版本發布狀態
- `#integration-success` → 系統整合成功狀態

#### 🔧 **自定義斜杠指令**
> **使用方式**: 輸入 /指令名 載入特定開發環境

**技術模組指令：**
- `/xsens` → 載入 XSENS MTi-680 開發環境
- `/mavlink` → 載入 MAVLink 協定開發環境
- `/debug-nmea` → 載入 NMEA 調試工具環境
- `/hardware` → 載入硬體整合開發環境

**每個指令會自動載入：**
- 📋 相關技術規格和配置
- 🔧 硬體連接和設定資訊
- 🐛 常見問題和解決方案
- 📁 相關檔案和程式碼片段
- 🧪 測試工具和除錯指令

### 🎛️ **個人設定**
- **代碼風格**: C++/Arduino, 4空格縮排, 中英混合註解, CamelCase函數名, snake_case變數名
- **版本管理**: feature-* 分支策略, 中英混合commit訊息, 每日增量提交
- **工作習慣**: 嵌入式系統專注, 硬體整合導向, 即時資料處理, 模組化設計
- **工具偏好**: Arduino IDE, C++標準, MAVLink協定, 硬體抽象層設計

---

