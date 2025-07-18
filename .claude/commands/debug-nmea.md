# /debug-nmea - NMEA 調試工具環境

## 🎯 上下文載入
當用戶輸入 `/debug-nmea` 時，自動載入以下上下文：

### 📋 NMEA 0183 規格
- **標準**: NMEA 0183 v4.0
- **格式**: ASCII 文字
- **結構**: $TALKER,DATA*CHECKSUM\r\n
- **最大長度**: 82 字元

### 🔧 支援的句子類型
**GPS 基本資料:**
- **GNGGA**: 全球定位系統定位資料
- **GNRMC**: 推薦最小定位資料
- **GNVTG**: 地面速度和航向
- **GNGSA**: GPS DOP 和活動衛星
- **GNGST**: GPS 偽距誤差統計
- **GNZDA**: 時間和日期

### 📊 資料解析流程
```cpp
String nmea_line = Serial4.readStringUntil('\n');
if (validateNMEAChecksum(nmea_line)) {
    parseNMEAMessage(nmea_line);
    forwardToXSENS(nmea_line);
}
```

### 🐛 調試工具
**1. Checksum 驗證器**
```cpp
bool validateNMEAChecksum(String sentence) {
    // 計算 XOR checksum
    // 比較計算值與句子中的值
}
```

**2. 句子解析器**
```cpp
void parseGNGGA(String sentence) {
    // 解析 GNGGA 句子
    // 提取時間、位置、品質指標
}
```

**3. 頻率監控器**
```cpp
void monitorNMEAFrequency() {
    // 監控各種句子的接收頻率
    // 統計成功/失敗率
}
```

### 📁 相關檔案
- **主程式**: Arduinopixhawkv00.ino (NMEA 處理部分)
- **調試輸出**: debug_output.txt
- **NMEA 範例**: nmea_samples.txt
- **錯誤日誌**: error_log.txt

### 🧪 測試 NMEA 句子
```
$GNGGA,123456.00,2512.12345,N,12112.12345,E,1,08,0.9,123.4,M,17.8,M,,*7A
$GNRMC,123456.00,A,2512.12345,N,12112.12345,E,0.0,0.0,180725,,,A*7C
$GNVTG,0.0,T,0.0,M,0.0,N,0.0,K,A*02
$GNGSA,A,3,01,02,03,04,05,06,07,08,,,,,2.1,0.9,1.9*3E
$GNGST,123456.00,12.3,4.5,6.7,8.9,10.1,11.2,13.4*5F
$GNZDA,123456.00,18,07,2025,00,00*76
```

### 🎯 常見問題診斷
**1. Checksum 錯誤**
- 檢查句子完整性
- 確認計算演算法正確
- 檢查特殊字元處理

**2. 解析失敗**
- 檢查欄位分隔符號
- 確認資料格式
- 檢查空白欄位處理

**3. 頻率不穩定**
- 檢查 UART 緩衝區
- 確認波特率設定
- 檢查資料流控制

### 🔍 調試指令
```cpp
// 啟用 NMEA 調試模式
#define DEBUG_NMEA 1

// 印出原始 NMEA 資料
Serial.println("RAW: " + nmea_line);

// 印出解析結果
Serial.printf("LAT: %.6f, LON: %.6f\n", latitude, longitude);

// 印出 checksum 計算
Serial.printf("CALC: %02X, RECV: %02X\n", calc_checksum, recv_checksum);
```

### 🎯 當前調試狀態
- **進度**: 調試中
- **問題**: checksum 驗證失敗
- **下一步**: 修復 validateNMEAChecksum() 函數