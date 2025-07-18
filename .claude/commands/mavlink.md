# /mavlink - MAVLink 協定開發環境

## 🎯 上下文載入
當用戶輸入 `/mavlink` 時，自動載入以下上下文：

### 📋 MAVLink 協定規格
- **版本**: MAVLink 2.0
- **系統ID**: 1 (Pixhawk)
- **組件ID**: 158 (MAV_COMP_ID_PERIPHERAL)
- **通訊介面**: UART (Serial1)
- **波特率**: 57600

### 🔧 硬體連接
```
Arduino ESP32 → Pixhawk
Serial1 TX   → TELEM2 RX
Serial1 RX   → TELEM2 TX
GND          → GND
```

### 📊 支援的訊息類型
**輸出訊息 (Arduino → Pixhawk):**
- **ODOMETRY (#331)**: 位置、速度、姿態資料
- **GPS_RAW_INT (#24)**: 原始 GPS 資料
- **GPS_INPUT (#232)**: 外部 GPS 輸入
- **VISION_POSITION_ESTIMATE (#102)**: 視覺位置估計
- **TIMESYNC (#111)**: 時間同步

**輸入訊息 (Pixhawk → Arduino):**
- **HEARTBEAT (#0)**: 系統心跳
- **SYSTEM_STATUS (#1)**: 系統狀態
- **ATTITUDE (#30)**: 姿態資料

### 🔄 資料流程
```
XSENS → Arduino → MAVLink → Pixhawk → PX4
```

### 📁 相關檔案
- **MAVLink 處理**: mavlink_handler.cpp
- **訊息定義**: mavlink_msg_*.h
- **座標轉換**: coordinate_transform.cpp
- **時間同步**: timesync_handler.cpp

### 🧪 測試指令
```cpp
// 測試 MAVLink 連接
mavlink_message_t msg;
mavlink_heartbeat_t heartbeat;
mavlink_msg_heartbeat_encode(1, 158, &msg, &heartbeat);
// 發送測試訊息
```

### 🎯 座標系統
- **XSENS**: NED (North-East-Down)
- **PX4**: NED (North-East-Down)
- **GPS**: LLH (Latitude-Longitude-Height)
- **轉換**: LLH → ENU → NED

### 🐛 常見問題
1. **訊息序列號錯誤**
   - 檢查 mavlink_msg_*_pack() 函數
   - 確認序列號遞增

2. **CRC 校驗失敗**
   - 檢查訊息長度
   - 確認訊息 ID 正確

3. **時間戳不同步**
   - 實施 TIMESYNC 機制
   - 校準系統時間

### 🎯 當前開發狀態
- **進度**: 85% 完成
- **支援訊息**: 5 種輸出訊息
- **測試狀態**: 硬體在環測試中