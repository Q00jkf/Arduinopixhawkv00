# 🎮 MINSPixhawk 指令參考手冊

> **版本**: 1.0  
> **更新日期**: 2025-07-18  
> **適用專案**: Arduinopixhawkv00  

## 📋 **快速索引**

| 類別 | 指令數量 | 快速跳轉 |
|------|----------|----------|
| [🎯 模式控制](#-模式控制) | 8 | AHRS, INS, IMU, ML_ODOM, ML_GNSS, ML_VISO, VEC, BIN |
| [🔧 系統設定](#-系統設定) | 6 | CONFIG, RESET, USB, CALI_GYRO, INIT_XSENS, INIT_LOCOSYS |
| [🐛 調試工具](#-調試工具) | 4 | DEBUG_ON, DEBUG_OFF, FLOW_STATUS, TRUST_STATUS |
| [🛡️ 安全控制](#-安全控制) | 4 | SECURITY_CHECK, SECURITY_ON, SECURITY_OFF, SECURITY_STATUS |
| [🎮 融合控制](#-融合控制) | 4 | FUSION_ON, FUSION_OFF, FUSION_STATUS, FUSION_DIAG |
| [📊 信任系統](#-信任系統) | 5 | TRUST_STATUS, TRUST_BASE, TRUST_RESET, TRUST_FORCE, TRUST_MAX |
| [⚙️ 頻率控制](#-頻率控制) | 2 | SET_FREQ, TEST_4HZ |
| [🧪 測試模式](#-測試模式) | 3 | GNSS_TEST, NMEA, XBUS |

---

## 🎯 **模式控制**

### `AHRS` 
**功能**: 設定 XSENS 輸出為 AHRS 模式  
**回應**: "Set Output Package to AHRS..."  
**用途**: 基本姿態和航向參考系統  

### `AHRS_QUT`
**功能**: 設定 XSENS 輸出為 AHRS 四元數模式  
**回應**: "Set Output Package to AHRS_QUT..."  
**用途**: 包含四元數的姿態參考系統  

### `INS`
**功能**: 設定 XSENS 輸出為 INS 模式  
**回應**: "Set Output Package to INS..."  
**用途**: 慣性導航系統模式  

### `INS_PAV_QUT`
**功能**: 設定 XSENS 輸出為 INS 位置-加速度-速度-四元數模式  
**回應**: "Set Output Package to INS_PAV_QUT..."  
**用途**: 完整慣性導航輸出  

### `IMU`
**功能**: 設定 XSENS 輸出為 IMU 模式  
**回應**: "Set Output Package to IMU..."  
**用途**: 慣性測量單元原始數據  

### `ML_ODOM`
**功能**: 設定 MAVLink ODOMETRY(331) 輸出  
**回應**: "Set MAVLINK ODOMETRY(331) Output..."  
**用途**: 裡程計數據輸出給 PX4  

### `ML_GNSS`
**功能**: 設定 MAVLink GPS_INPUT(232) 輸出  
**回應**: "Set MAVLINK GPS_INPUT(232) Output..."  
**用途**: GNSS 數據輸出給 PX4  

### `ML_VISO`
**功能**: 設定 MAVLink VISION_POSITION_ESTIMATE(102) 輸出  
**回應**: "Set MAVLINK VISION_POSITION_ESTIMATE(102) Output..."  
**用途**: 視覺位置估計輸出  

### `VEC`
**功能**: 發送 VectorNav 兼容指令  
**回應**: "$VNRRG,01,VN-300*58"  
**用途**: VectorNav 感測器兼容模式  

### `BIN`
**功能**: 設定二進制輸出模式  
**回應**: "Set Binary Output..."  
**用途**: 高效率的二進制數據傳輸  

---

## 🔧 **系統設定**

### `CONFIG`
**功能**: 進入配置模式  
**回應**: "Set CONFIG Mode..."  
**用途**: 系統配置和參數設定  

### `RESET`
**功能**: 重置 XSENS 感測器  
**回應**: "Reseting..."  
**用途**: 硬體重置感測器  

### `USB`
**功能**: 進入 USB 配置模式  
**回應**: "Enter USB Configuration Mode..."  
**用途**: 通過 USB 進行系統配置  

### `USB_OUT`
**功能**: 離開 USB 配置模式  
**回應**: "Leave USB Configuration Mode..."  
**用途**: 退出 USB 配置模式  

### `CALI_GYRO`
**功能**: 校準陀螺儀  
**回應**: 校準進度訊息  
**用途**: 10 秒陀螺儀校準程序  

### `INIT_XSENS` ⚠️
**功能**: 初始化 XSENS MTI-680 感測器  
**限制**: 僅在 USB 配置模式下可用  
**用途**: 設定 XSENS 參數和方向  

### `INIT_LOCOSYS` ⚠️
**功能**: 初始化 LOCOSYS GNSS 接收器  
**限制**: 僅在 USB 配置模式下可用  
**用途**: 配置 GNSS 接收器參數  

---

## 🐛 **調試工具**

### `DEBUG_ON`
**功能**: 啟用調試模式  
**回應**: "Debug Mode ON - NMEA data output enabled"  
**用途**: 開啟 NMEA 數據輸出和調試訊息  

### `DEBUG_OFF`
**功能**: 關閉調試模式  
**回應**: "Debug Mode OFF - NMEA data output disabled"  
**用途**: 關閉調試訊息以提高性能  

### `FLOW_STATUS`
**功能**: 顯示數據流狀態  
**回應**: 詳細的數據流信息  
**用途**: 監控 MAVLink 頻率和數據流狀態  

### `TRUST_STATUS`
**功能**: 顯示動態信任系統狀態  
**回應**: 運動評分、信任因子等詳細信息  
**用途**: 監控 EKF 動態權重系統  

---

## 🛡️ **安全控制**

### `SECURITY_CHECK [token]`
**功能**: 執行安全檢查  
**參數**: 安全令牌 (可選)  
**限制**: 5 秒速率限制  
**用途**: 驗證系統安全性  

### `SECURITY_ON [token]`
**功能**: 啟用安全模式  
**參數**: 安全令牌 (必需)  
**限制**: 5 秒速率限制  
**用途**: 啟用增強安全保護  

### `SECURITY_OFF [token]`
**功能**: 關閉安全模式  
**參數**: 安全令牌 (必需)  
**限制**: 5 秒速率限制  
**用途**: 關閉安全保護  

### `SECURITY_STATUS`
**功能**: 查詢安全模式狀態  
**回應**: 當前安全模式狀態  
**用途**: 檢查安全模式是否啟用  

---

## 🎮 **融合控制**

### `FUSION_ON`
**功能**: 啟用 GNSS-AHRS 融合模式  
**回應**: "GNSS-AHRS Fusion Mode ENABLED"  
**用途**: 啟用感測器融合演算法  

### `FUSION_OFF`
**功能**: 關閉 GNSS-AHRS 融合模式  
**回應**: "GNSS-AHRS Fusion Mode DISABLED"  
**用途**: 關閉感測器融合演算法  

### `FUSION_STATUS`
**功能**: 查詢融合模式狀態  
**回應**: 融合模式狀態和統計訊息  
**用途**: 監控融合演算法運行狀態  

### `FUSION_DIAG`
**功能**: 執行融合系統診斷  
**回應**: 詳細的診斷訊息  
**用途**: 診斷融合演算法問題  

---

## 📊 **信任系統**

### `TRUST_STATUS`
**功能**: 顯示動態信任系統詳細狀態  
**回應**: 運動評分、信任因子、YAW 穩定性等  
**用途**: 監控動態權重系統  

### `TRUST_BASE [value]`
**功能**: 設定基礎信任因子  
**參數**: 0.1-1.0 之間的浮點數  
**用途**: 調整 EKF 基礎信任度  

### `TRUST_RESET`
**功能**: 重置信任系統  
**回應**: "Dynamic Trust System Reset"  
**用途**: 重新初始化信任系統  

### `TRUST_FORCE [value]`
**功能**: 強制設定信任因子  
**參數**: 0.1-1.0 之間的浮點數  
**用途**: 手動覆蓋信任因子  

### `TRUST_MAX`
**功能**: 設定最大信任因子  
**回應**: "Trust Factor set to MAXIMUM"  
**用途**: 設定最高信任度  

---

## ⚙️ **頻率控制**

### `SET_FREQ [Hz]`
**功能**: 設定 MAVLink 輸出頻率  
**參數**: 10-100 Hz 之間的整數  
**用途**: 調整數據輸出頻率  

### `TEST_4HZ`
**功能**: 測試 LOCOSYS 4Hz 配置  
**回應**: 測試結果和配置狀態  
**用途**: 驗證 GNSS 4Hz 設定  

---

## 🧪 **測試模式**

### `GNSS_TEST`
**功能**: 啟用 GNSS 測試模式  
**回應**: "Set GNSS Test Mode..."  
**用途**: 測試 GNSS 接收器功能  

### `NMEA`
**功能**: 啟用 NMEA 輸出模式  
**回應**: "Set NMEA Mode..."  
**用途**: 輸出標準 NMEA 0183 格式  

### `XBUS`
**功能**: 啟用 XBUS 協定模式  
**回應**: "Set XBUS protocal..."  
**用途**: 使用 XSENS 原生 XBUS 協定  

---

## 🎯 **使用建議**

### **常用工作流程**
```bash
# 1. 系統啟動
USB → INIT_XSENS → INIT_LOCOSYS → USB_OUT

# 2. 基本操作
AHRS → ML_ODOM → DEBUG_ON

# 3. 調試流程
DEBUG_ON → TRUST_STATUS → FUSION_STATUS → FLOW_STATUS

# 4. 安全檢查
SECURITY_CHECK → SECURITY_STATUS
```

### **性能優化**
- 生產環境建議關閉 DEBUG_OFF
- 高頻率應用使用 BIN 模式
- 使用 TRUST_MAX 獲得最佳性能

### **故障排除**
- 使用 RESET 解決感測器問題
- 使用 FUSION_DIAG 診斷融合問題
- 使用 TRUST_RESET 重置信任系統

---

## 📝 **注意事項**

⚠️ **安全提醒**
- 安全相關指令有 5 秒速率限制
- 某些配置指令僅在 USB 模式下可用
- 建議在測試環境中先驗證指令效果

🔧 **技術限制**
- 頻率設定範圍: 10-100 Hz
- 信任因子範圍: 0.1-1.0
- 部分指令需要特定硬體支援

📊 **性能建議**
- 使用 BIN 模式獲得最佳性能
- 在生產環境中關閉 DEBUG 模式
- 根據應用需求調整輸出頻率