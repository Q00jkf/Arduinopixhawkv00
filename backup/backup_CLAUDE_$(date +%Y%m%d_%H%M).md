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
src/main/cpp/
├── core/           # Core system logic and mode switching
├── sensors/        # Xsens sensor communication and data parsing
├── mavlink/        # MAVLink message handling and transmission
├── utils/          # Coordinate transformation and mathematical utilities
├── models/         # Data structures and sensor models
├── services/       # System services and initialization
└── api/            # External interfaces and communication protocols
```

### 🎯 **DEVELOPMENT STATUS**
- **Setup**: ✅ Complete
- **Core Features**: 🔄 In Development
- **Testing**: ⏳ Pending
- **Documentation**: 🔄 In Progress

---

## 📅 2025-07-10 工作成果總結

### ✅ **今日完成的重要成果**

#### 🎯 **1. NMEA句子類型擴展 (已完成並合併到dev)**
- **新增支援**: GNGSA, GNVTG, GNZDA 三種NMEA句子類型
- **功能實現**: 
  - GNGSA (⭐⭐⭐⭐⭐): 衛星狀態和精度因子
  - GNVTG (⭐⭐⭐): 地面速度和航向
  - GNZDA (⭐⭐⭐): UTC時間同步
- **Git狀態**: ✅ 已提交到feature-XSENSconnect → ✅ 已合併到dev → ✅ 已推送到GitHub

#### 🔧 **2. 獨立頻率控制系統 (已實現)**
- **新增變數**: 
  ```cpp
  unsigned long last_gsa_send = 0;  // GSA句子頻率控制
  unsigned long last_vtg_send = 0;  // VTG句子頻率控制  
  unsigned long last_zda_send = 0;  // ZDA句子頻率控制
  unsigned long gsa_sent = 0, vtg_sent = 0, zda_sent = 0; // 統計計數器
  ```
- **邏輯優化**: 每種句子類型獨立1Hz頻率控制
- **監控增強**: 新增TIMING2顯示，涵蓋所有6種句子類型

#### 📊 **3. 問題診斷與分析 (已識別)**
- **VTG/ZDA為0原因**: GNSS模組預設未啟用這些句子輸出
- **MTi-680警告**: "Incomplete dataset for the GNSS module" 持續出現
- **根本問題**: 懷疑NMEA checksum驗證邏輯有問題

#### 🔍 **4. 系統運行狀態分析**
- **成功句子**: GNGGA(711), GNRMC(686), GNGST(690), GNGSA(880)
- **成功率**: 10.1% (29518接收, 2967發送)
- **問題現象**: VTG=0, ZDA=0 (GNSS模組未輸出)

---

## 🎯 明天的優先任務清單

### 🔥 **最高優先級 - NMEA Checksum驗證修復**

#### **任務1: 修復NMEA checksum驗證函數**
```cpp
// 位置: Arduinopixhawkv00.ino:1464-1481
// 問題: validateNMEAChecksum() 可能有邏輯錯誤
// 行動: 添加詳細調試輸出，分析實際checksum計算過程
```

**具體步驟:**
1. **添加詳細調試輸出** - 顯示checksum計算過程
2. **測試實際NMEA句子** - 用日誌中的真實句子驗證
3. **修復驗證邏輯** - 根據調試結果修正算法
4. **驗證修復效果** - 觀察MTi-680警告是否消失

#### **任務2: GNSS模組VTG/ZDA啟用 (可選)**
```cpp
// 如果checksum修復後問題仍存在，考慮啟用額外句子
// 位置: setup()函數中添加GNSS配置命令
Serial4.println("$PUBX,40,VTG,0,1,0,0,0,0*5E");  // 啟用VTG
Serial4.println("$PUBX,40,ZDA,0,1,0,0,0,0*44");  // 啟用ZDA
```

### 🔧 **次要優先級 - 系統優化**

#### **任務3: 增強錯誤監控**
- 改善無效NMEA句子的處理和顯示
- 添加checksum失敗統計
- 提供更詳細的調試信息

#### **任務4: 性能優化**
- 檢查NMEA處理效率
- 優化句子過濾邏輯
- 減少不必要的字符串操作

---

## 🚀 明天的啟動指令

### **快速開始工作:**
```bash
# 1. 切換到工作目錄
cd /mnt/c/Users/user/MINSPixhawk/Arduinopixhawkv00

# 2. 檢查當前分支狀態  
git status
git branch

# 3. 如果需要，創建新的調試分支
git checkout -b fix-nmea-checksum

# 4. 開始checksum調試
# 重點檢查: Arduinopixhawkv00.ino:1464-1481
```

### **調試重點文件:**
- **主要**: `Arduinopixhawkv00.ino` (checksum驗證函數)
- **參考**: 實際NMEA句子日誌 (用於測試驗證)
- **監控**: Serial輸出中的checksum錯誤信息

### **預期解決方案:**
1. **立即調試** - 添加詳細checksum計算日誌
2. **問題識別** - 找出驗證邏輯的具體問題  
3. **快速修復** - 修正checksum算法
4. **驗證結果** - 觀察MTi-680警告消失

---

## 📋 MINSPIXHAWK-SPECIFIC DEVELOPMENT GUIDELINES

### 🔧 **C++/Arduino SPECIFIC RULES**
- **Header organization**: `.h` files in appropriate modules, implementations in corresponding `.cpp`
- **Class structure**: Use namespaces for organizational separation (e.g., `MyQuaternion::`, `MyDirectCosineMatrix::`)
- **Memory management**: Embedded-friendly code, avoid dynamic allocation where possible
- **Hardware abstractions**: UART operations through proper abstraction layers

### 📡 **MAVLINK & SENSOR INTEGRATION**
- **Protocol compliance**: Ensure MAVLink message format compliance
- **Timing critical**: Maintain real-time performance for sensor data processing
- **State management**: Proper mode switching and system state transitions
- **Error handling**: Robust error detection and recovery mechanisms

### 🧪 **TESTING APPROACH**
- **Unit tests**: Individual component testing in `src/test/unit/`
- **Integration tests**: Full system integration in `src/test/integration/`
- **Hardware-in-loop**: Physical testing with actual Xsens and Pixhawk hardware

## 🎯 RULE COMPLIANCE CHECK

Before starting ANY task, verify:
- [ ] ✅ I acknowledge all critical rules above
- [ ] Files go in proper module structure (src/main/cpp/)
- [ ] Use Task agents for >30 second operations
- [ ] TodoWrite for 3+ step tasks
- [ ] Commit after each completed task
- [ ] Push to GitHub after every commit

## 🚀 COMMON COMMANDS

```bash
# Build project (when build system is configured)
# make build

# Upload to Arduino (when configured)
# make upload

# Run tests (when test framework is configured)
# make test

# Monitor serial output
# make monitor

# Check current git branch
git -C /mnt/c/Users/user/MINSPixhawk/Arduinopixhawkv00 branch
```

## 📋 WORK SESSION REPORTING
- **After each work session**, report current branch status
- **Working directory**: `/mnt/c/Users/user/MINSPixhawk/Arduinopixhawkv00`
- **Current branch**: `dev` (最新更新包含NMEA擴展功能)

## 🚨 TECHNICAL DEBT PREVENTION

### ❌ WRONG APPROACH (Creates Technical Debt):
```bash
# Creating new file without searching first
Write(file_path="new_sensor_v2.cpp", content="...")
```

### ✅ CORRECT APPROACH (Prevents Technical Debt):
```bash
# 1. SEARCH FIRST
Grep(pattern="sensor.*implementation", include="*.cpp")
# 2. READ EXISTING FILES  
Read(file_path="src/main/cpp/sensors/xsens_sensor.cpp")
# 3. EXTEND EXISTING FUNCTIONALITY
Edit(file_path="src/main/cpp/sensors/xsens_sensor.cpp", old_string="...", new_string="...")
```

## 🧹 DEBT PREVENTION WORKFLOW

### Before Creating ANY New File:
1. **🔍 Search First** - Use Grep/Glob to find existing implementations
2. **📋 Analyze Existing** - Read and understand current patterns
3. **🤔 Decision Tree**: Can extend existing? → DO IT | Must create new? → Document why
4. **✅ Follow Patterns** - Use established project patterns
5. **📈 Validate** - Ensure no duplication or technical debt

---

**⚠️ Prevention is better than consolidation - build clean from the start.**  
**🎯 Focus on single source of truth and extending existing functionality.**  
**📈 Each task should maintain clean architecture and prevent technical debt.**

---

## 📅 2025-07-14 工作日誌與明天計劃

### ✅ **今日完成的重要成果**

#### 🎯 **1. 問題根源識別 (已確認)**
- **XSENS工程師反饋**: MTi-680不願意讀取資料，認為輸入頻率未達到4Hz
- **技術分析**: 目前是**主動推送模式**，可能與XSENS內部時間軸不匹配
- **問題假設**: GNSS資料推送時序與XSENS採樣週期不同步

#### 🔧 **2. 頻率控制完全移除 (已實現)**
- **分支**: `feature-same-frequence` (基於乾淨的dev分支)
- **修改**: 移除所有MCU端NMEA頻率限制邏輯
- **效果**: 實現LOCOSYS → Arduino → MTi-680的直接轉發
- **Git狀態**: ✅ 已提交 commit `4762b14`

#### 📊 **3. XSENS API研究 (已調查)**
- **LOCOSYS硬體**: RTK-DUAL-A支援最高5Hz，4Hz在規格內
- **XSENS API限制**: 沒有發現pull模式或請求-回應機制
- **現有配置**: 僅支援推送模式 `setGnssReceiverSettings(baudrate, update_rate, talker_id)`

#### 🔍 **4. 問題分析與假設 (已識別)**
- **時序不匹配**: Arduino主動推送 vs XSENS內部採樣週期
- **緩衝區衝突**: 推送時間可能與XSENS準備接收時間不符
- **頻率問題**: 雖然移除了Arduino限制，但推送模式本身可能是問題

---

## 🚀 明天的優先任務清單

### 🔥 **最高優先級 - 時序同步解決方案**

#### **方案A: XSENS狀態監控機制**
```cpp
// 檢查XSENS是否準備接收GNSS數據
if (xsens.isReadyForGNSS() || xsens_data_request_received) {
    sendNMEAtoXSENS(nmea_sentence);
}
```

**實施步驟:**
1. **研究XSENS狀態查詢** - 檢查API中是否有狀態查詢功能
2. **監控XSENS回應** - 分析XSENS是否發送請求信號
3. **實現條件發送** - 只在XSENS準備好時發送NMEA

#### **方案B: 時序同步緩衝區**
```cpp
// NMEA數據緩衝區，按XSENS節奏發送
queue<String> nmea_buffer;
unsigned long last_xsens_sync = 0;
const unsigned long XSENS_SYNC_INTERVAL = 250; // 4Hz = 250ms

if (millis() - last_xsens_sync >= XSENS_SYNC_INTERVAL) {
    if (!nmea_buffer.empty()) {
        sendLatestNMEAtoXSENS();
        last_xsens_sync = millis();
    }
}
```

**實施步驟:**
1. **實現NMEA緩衝隊列** - 暫存接收到的NMEA句子
2. **建立XSENS同步計時器** - 按XSENS預期頻率發送
3. **最新數據優先** - 確保發送最新的GNSS數據

#### **方案C: XSENS時間軸對齊**
```cpp
// 監聽XSENS的數據輸出時序，在其間隙發送NMEA
if (xsens_data_output_gap_detected) {
    sendNMEAimmediately();
}
```

**實施步驟:**
1. **分析XSENS輸出模式** - 觀察MTi-680數據輸出時序
2. **找出數據間隙** - 識別XSENS準備接收的時機
3. **時機化發送** - 在適當時機推送NMEA數據

### 🔧 **次要優先級 - 系統優化**

#### **任務1: 數據流分析**
- 記錄完整的數據時序圖
- 分析XSENS和LOCOSYS的時間關係
- 識別潛在的時序衝突點

#### **任務2: 調試增強**
- 添加詳細的時序日誌
- 監控XSENS接收狀態
- 記錄數據丟失或拒絕的時機

---

## 🎯 明天的啟動指令

### **快速開始工作:**
```bash
# 1. 切換到工作目錄和正確分支
cd /mnt/c/Users/user/MINSPixhawk/Arduinopixhawkv00
git checkout feature-same-frequence

# 2. 檢查當前狀態
git status
git log --oneline -3

# 3. 開始實施時序同步方案
# 優先嘗試方案B: 時序同步緩衝區
```

### **重點研究方向:**
1. **XSENS API深度分析** - 查找任何狀態查詢或同步機制
2. **時序緩衝實現** - 建立智能NMEA緩衝區
3. **數據流優化** - 確保在XSENS準備好時發送數據

### **預期目標:**
- **解決時序不匹配問題**
- **實現XSENS穩定接收4Hz GNSS數據**
- **通過XSENS工程師的驗證要求**

### **成功指標:**
- XSENS不再出現"不願意讀資料"的問題
- 穩定的4Hz GNSS數據流
- MTi-680正常融合GNSS和INS數據

---

**🌅 明天重點: 從推送模式的時序問題入手，實現XSENS友好的數據傳輸機制！**