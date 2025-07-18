# /hardware - 硬體整合開發環境

## 🎯 上下文載入
當用戶輸入 `/hardware` 時，自動載入以下上下文：

### 📋 硬體架構
**核心控制單元:**
- **MCU**: Arduino ESP32
- **CPU**: 雙核 240MHz
- **記憶體**: 520KB SRAM, 4MB Flash
- **通訊**: 3x UART, I2C, SPI

**感測器模組:**
- **IMU/GPS**: XSENS MTi-680
- **GNSS**: LOCOSYS RTK-DUAL-A
- **飛控**: Pixhawk (PX4)

### 🔌 連接拓撲
```
┌─────────────────────────────────────────────────────────────┐
│                    Arduino ESP32                           │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐  │
│  │   Serial1   │   Serial2   │   Serial3   │   Serial4   │  │
│  │  (MAVLink)  │   (XSENS)   │ (NMEA OUT)  │ (NMEA IN)   │  │
│  └─────────────┴─────────────┴─────────────┴─────────────┘  │
└─────────────────────────────────────────────────────────────┘
         │             │             │             │
         │             │             │             │
    ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐
    │Pixhawk  │   │ MTi-680 │   │ Monitor │   │ LOCOSYS │
    │PX4 FC   │   │ XSENS   │   │ Output  │   │RTK-DUAL │
    └─────────┘   └─────────┘   └─────────┘   └─────────┘
```

### ⚡ 電源系統
- **輸入電壓**: 5V DC
- **Arduino 功耗**: ~500mA
- **XSENS 功耗**: ~300mA
- **LOCOSYS 功耗**: ~200mA
- **總功耗**: ~1A @ 5V

### 🔧 UART 配置
```cpp
// Serial1: MAVLink (Pixhawk)
Serial1.begin(57600);

// Serial2: XSENS MTi-680
Serial2.begin(115200);

// Serial3: NMEA 輸出 (監控)
Serial3.begin(9600);

// Serial4: NMEA 輸入 (LOCOSYS)
Serial4.begin(9600);
```

### 📊 資料流向
```
LOCOSYS → Serial4 → Arduino → Serial2 → XSENS
                     ↓
                 Serial1 → Pixhawk
                     ↓
                 Serial3 → Monitor
```

### 🐛 硬體常見問題
**1. 通訊中斷**
- 檢查接線是否正確
- 確認波特率設定
- 檢查電源供應

**2. 資料丟失**
- 增加 UART 緩衝區
- 降低資料傳輸率
- 檢查中斷處理

**3. 時序問題**
- 同步化資料流
- 實施硬體時鐘
- 優化中斷優先級

### 🧪 硬體測試
**1. 連接測試**
```cpp
void testHardwareConnections() {
    Serial.println("Testing hardware connections...");
    
    // 測試 UART 連接
    if (Serial1.available()) Serial.println("Serial1 OK");
    if (Serial2.available()) Serial.println("Serial2 OK");
    if (Serial3.available()) Serial.println("Serial3 OK");
    if (Serial4.available()) Serial.println("Serial4 OK");
}
```

**2. 資料流測試**
```cpp
void testDataFlow() {
    // 測試 NMEA 資料流
    String nmea = Serial4.readString();
    Serial2.print(nmea);  // 轉發到 XSENS
    Serial3.print(nmea);  // 輸出到監控
}
```

**3. 效能測試**
```cpp
void testPerformance() {
    unsigned long start = micros();
    // 執行資料處理
    unsigned long end = micros();
    Serial.printf("Processing time: %lu us\n", end - start);
}
```

### 📁 相關檔案
- **硬體初始化**: hardware_init.cpp
- **UART 設定**: uart_config.h
- **中斷處理**: interrupt_handler.cpp
- **電源管理**: power_management.cpp

### 🎯 硬體規格限制
- **最大 UART 速度**: 2Mbps
- **最大中斷頻率**: 1kHz
- **記憶體使用限制**: 80% (416KB)
- **CPU 使用限制**: 80% (雙核)

### 🔍 除錯工具
```cpp
// 硬體狀態監控
void printHardwareStatus() {
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("CPU freq: %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash size: %u\n", ESP.getFlashChipSize());
}

// UART 狀態檢查
void checkUARTStatus() {
    Serial.printf("Serial1 available: %d\n", Serial1.available());
    Serial.printf("Serial2 available: %d\n", Serial2.available());
    Serial.printf("Serial3 available: %d\n", Serial3.available());
    Serial.printf("Serial4 available: %d\n", Serial4.available());
}
```

### 🎯 當前硬體狀態
- **進度**: 95% 完成
- **測試狀態**: 硬體在環測試中
- **問題**: 間歇性資料丟失
- **下一步**: 優化 UART 緩衝區管理