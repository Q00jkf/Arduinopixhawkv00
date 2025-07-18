# /xsens - XSENS MTi-680 開發環境

## 🎯 上下文載入
當用戶輸入 `/xsens` 時，自動載入以下上下文：

### 📋 技術規格
- **感測器型號**: XSENS MTi-680
- **通訊協定**: XBUS + NMEA 0183
- **資料更新率**: 100Hz (IMU), 4Hz (GNSS)
- **電源需求**: 5V DC
- **通訊介面**: UART (Serial2)

### 🔧 硬體連接
```
Arduino ESP32 → XSENS MTi-680
Serial2 TX   → XSENS RX
Serial2 RX   → XSENS TX
5V           → VCC
GND          → GND
```

### 📊 資料格式
**XBUS 二進位格式:**
- 加速度: float[3] (m/s²)
- 角速度: float[3] (rad/s)
- 磁場: float[3] (Gauss)
- 四元數: float[4] (w,x,y,z)
- GPS 位置: double[3] (lat,lon,alt)

**NMEA 格式:**
- GNGGA: GPS 位置和品質
- GNRMC: 推薦最小資料
- GNVTG: 地面速度和航向
- GNGSA: 衛星狀態
- GNGST: GPS 偽距誤差統計
- GNZDA: 時間和日期

### 🐛 常見問題
1. **時序同步問題**
   - 原因: Arduino 推送頻率與 XSENS 採樣不匹配
   - 解決: 實施 250ms 緩衝區機制

2. **NMEA Checksum 錯誤**
   - 原因: 校驗和計算錯誤
   - 解決: 檢查 validateNMEAChecksum() 函數

3. **資料丟失**
   - 原因: UART 緩衝區溢出
   - 解決: 增加緩衝區大小或降低資料率

### 📁 相關檔案
- **主程式**: Arduinopixhawkv00.ino
- **UART 通訊**: myUARTSensor.cpp
- **資料融合**: gnss_ahrs_fusion.h
- **座標轉換**: coordinate_transform.cpp

### 🧪 測試指令
```cpp
// 測試 XSENS 連接
Serial2.begin(115200);
// 測試資料讀取
if (xsens.readData()) {
    Serial.println("XSENS 資料正常");
}
```

### 🎯 當前開發狀態
- **進度**: 80% 完成
- **當前問題**: 時序同步機制
- **下一步**: 實施緩衝區解決方案