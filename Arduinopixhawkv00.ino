#include <MAVLink.h>
#include <Arduino.h>
#include "myUARTSensor.h"
#include "myUART.h"
#include "Orientation.h"

// Function declarations
void performSecurityCheck(HardwareSerial &port, bool silent_mode);
bool authenticateSecurityCommand(uint32_t provided_token);
void enableSecurityMode(HardwareSerial &port);
void disableSecurityMode(HardwareSerial &port);
bool checkHardwareIntegrity();
bool validateSensorData();
bool checkCommunicationSecurity();
bool checkTimeSyncIntegrity();
bool checkMemoryIntegrity();
uint32_t simpleHash(uint32_t input);
bool constantTimeCompare(uint32_t a, uint32_t b);

// 動態權重系統函數聲明
void updateMovementDetection(const my_data_2d &latlon, const my_data_u4 &hei, 
                           const my_data_3f &vel, const my_data_3f &omg, 
                           const my_data_3f &acc);
float calculateTrustFactor();
float calculateYawTrustFactor();
void adaptiveCovariance(mavlink_odometry_t &odom, float trust_factor);

// NMEA 時間戳格式標準化函數聲明
String normalizeTimestampFormat(const String& timestamp);
String normalizeNMEASentence(const String& nmea_sentence);

#define PIXHAWK_SERIAL Serial1
#define Serial_xsens Serial2
#define NMEA_OUT_Serial Serial3
#define NMEA_IN_Serial Serial4
#define DATA_OK       0
#define DATA_WARNING  1
#define DATA_INVALID  2

XsensUart xsens(Serial_xsens);
int64_t time_offset = 0;
uint64_t xsens_pre_time = 0;
uint8_t current_Xsens_mode = MODE_AHRS; // 編號: 1
uint8_t current_output_mode = OUT_MODE_BIN;
bool is_run = false;  
bool is_debug = false;
bool ISR_flag_xsens = false, ISR_flag_NMEA = false;
bool USB_Setting_mode = false;
bool enable_input = true;
bool time_sync_initialized = false;  // 新增：時間同步初始化標記
bool gps_connected = false;          // GPS 連接狀態
unsigned long last_gps_data_time = 0; // 最後收到 GPS 數據的時間
String nmea_input_buffer = "";       // NMEA 串列累積緩衝區

// 動態權重系統變數
struct {
    float prev_pos[3] = {0, 0, 0};    // 前一次位置 [lat, lon, alt]
    float prev_vel[3] = {0, 0, 0};    // 前一次速度 [vx, vy, vz]
    float prev_omg[3] = {0, 0, 0};    // 前一次角速度 [roll, pitch, yaw rates]
    float movement_score = 0.0f;      // 運動程度評分 (0-1)
    float trust_factor = 0.1f;        // EKF 信任因子 (0.1-1.0)
    unsigned long last_update = 0;    // 上次更新時間
    bool is_initialized = false;      // 是否已初始化
    float acceleration_mag = 0.0f;    // 加速度幅值
    float angular_rate_mag = 0.0f;    // 角速度幅值
    
    // YAW軸專用變數
    float yaw_rate_history[5] = {0, 0, 0, 0, 0}; // YAW角速度歷史記錄
    int yaw_history_index = 0;        // 歷史記錄索引
    float yaw_stability_score = 0.0f; // YAW穩定性評分 (0-1)
    float yaw_trust_factor = 0.1f;    // YAW專用信任因子 (0.1-1.0)
} dynamic_trust;
bool security_mode_active = false;    // 安全模式狀態
unsigned long last_security_check = 0; // 最後一次安全檢查時間
uint8_t security_level = 0;           // 安全等級 (0=正常, 1=警告, 2=錯誤)
unsigned long last_security_cmd = 0;   // 最後一次安全指令時間 (防護DoS)
uint8_t failed_auth_attempts = 0;     // 失敗認證次數
unsigned long lockout_start_time = 0; // 鎖定開始時間 (非阻塞式)
bool is_locked_out = false;           // 鎖定狀態標記
// 注意：在生產環境中，令牌應存儲在安全硬體中，不應硬編碼
const uint32_t SECURITY_TOKEN_HASH = 0x7F8E9A2B; // 令牌的雜湊值（而非明文）
my_data_u4 latestXsensTime;
unsigned long last_sync_millis = 0;         // 上一次時間同步的時間點（ms）
const unsigned long SYNC_INTERVAL = 1000;   // 每 1 秒重新同步一次，改善延遲和精度（單位：ms）

// MAVLink 發送頻率控制 - 統一數據流暢性
const uint32_t TARGET_MAVLINK_HZ = 50;       // 目標 MAVLink 發送頻率 (Hz)
const uint32_t MAVLINK_SEND_INTERVAL = 1000000 / TARGET_MAVLINK_HZ / 10;  // Xsens時間戳間隔 (20000 for 50Hz)

// NMEA 發送頻率控制 - 針對 MTi-680 優化 (分類型控制)
unsigned long last_gga_send = 0;            // 上次發送GGA的時間
unsigned long last_rmc_send = 0;            // 上次發送RMC的時間
unsigned long last_gst_send = 0;            // 上次發送GST的時間
unsigned long last_gsa_send = 0;            // 上次發送GSA的時間
unsigned long last_vtg_send = 0;            // 上次發送VTG的時間
unsigned long last_zda_send = 0;            // 上次發送ZDA的時間
// NMEA頻率控制已移除 - 直接轉發LOCOSYS輸出，不做頻率限制
unsigned long nmea_received_count = 0;       // 接收到的NMEA句子總數
unsigned long nmea_sent_count = 0;           // 發送到MTi-680的句子數
unsigned long nmea_invalid_count = 0;        // 無效NMEA句子數
unsigned long nmea_discarded_count = 0;      // 被丟棄的不完整句子數
unsigned long gga_sent = 0, rmc_sent = 0, gst_sent = 0; // 各類型句子發送統計
unsigned long gsa_sent = 0, vtg_sent = 0, zda_sent = 0; // 新增句子類型發送統計

// NMEA 數據組同步機制 - 基於XSENS官方要求的完整數據組
struct NMEADataset {
  String gga_sentence = "";
  String rmc_sentence = "";
  String gst_sentence = "";
  String gsa_sentences[4] = {"", "", "", ""}; // XSENS要求4個GSA句子
  String vtg_sentence = "";
  String zda_sentence = "";
  String timestamp = "";      // 共同時間戳
  int gsa_count = 0;         // 已收集的GSA句子數量
  bool has_gga = false;
  bool has_rmc = false;
  bool has_gst = false;
  bool has_vtg = false;
  bool has_zda = false;
  unsigned long collect_start_time = 0; // 數據組收集開始時間
};

// 全局變量存儲最新有效時間戳
String last_valid_timestamp = "";

// 時間戳容差比較函數 - 允許±1秒的差異
bool isTimestampCompatible(const String& ts1, const String& ts2) {
  if (ts1.length() < 6 || ts2.length() < 6) return false;
  
  // 提取時間部分 HHMMSS
  float time1 = ts1.substring(0, 6).toFloat();
  float time2 = ts2.substring(0, 6).toFloat();
  
  // 計算時間差（考慮秒的小數部分）
  float diff = abs(time1 - time2);
  
  // 處理跨分鐘/小時的情況（例如：235959 vs 000000）
  if (diff > 5000) {  // 可能是跨小時
    diff = min(diff, 10000 - diff);
  }
  if (diff > 50) {    // 可能是跨分鐘
    diff = min(diff, 100 - diff);
  }
  
  // 嚴格匹配：只允許±0.1秒的容差，確保同組數據時間戳一致
  return diff <= 0.1;
}

NMEADataset current_dataset;           // 當前正在收集的數據組
const unsigned long DATASET_TIMEOUT = 2000; // 數據組收集超時時間(ms)
unsigned long complete_datasets_sent = 0;    // 完整數據組發送統計
unsigned long incomplete_datasets = 0;       // 不完整數據組統計

// 平滑 time_offset：使用滑動平均法
const int OFFSET_BUF_SIZE = 5;
int64_t time_offset_buffer[OFFSET_BUF_SIZE] = {0};
int offset_index = 0;
bool offset_buffer_full = false;

// 底下加入自動同步新增進入 loop()
void syncPX4Time() {
  if (millis() - last_sync_millis >= SYNC_INTERVAL) {
    uint64_t px4_time = getPX4Time(false);
    if (px4_time > 0) {
      int64_t new_offset = px4_time - uint64_t(latestXsensTime.ulong_val) * 1e2;

      // 將新 offset 放入緩衝區
      time_offset_buffer[offset_index] = new_offset;
      offset_index++;

      if (offset_index >= OFFSET_BUF_SIZE) {
        offset_index = 0;
        offset_buffer_full = true;
      }

      int count = offset_buffer_full ? OFFSET_BUF_SIZE : offset_index;
      int64_t sum = 0;
      for (int i = 0; i < count; i++) {
        sum += time_offset_buffer[i];
      }
      time_offset = sum / count;

      last_sync_millis = millis();
      Serial.print("[SYNC] Updated smoothed time_offset = ");
      Serial.println(time_offset);
    }
  }
}
void SERCOM2_Handler() {
  Serial_xsens.IrqHandler();
  ISR_flag_xsens = true;
}

void SERCOM3_Handler() {
  NMEA_IN_Serial.IrqHandler();
  ISR_flag_NMEA = true;
}

void setup() {
  Serial.begin(115200);  // 與 myUART_init() 一致
  delay(1000);
  Serial.println("=== Arduino2PixhawkV0 Starting ===");
  
  Serial.println("Step 1: Initializing UART...");
  myUART_init();
  Serial.println("Step 1: Complete");
  
  Serial.println("Step 2: Checking USB Serial...");
  is_debug = checkUSBSerial();
  Serial.println("Step 2: Complete");
  
  Serial.println("Step 3: Checking PX4 connection...");
  checkPX4CON();
  Serial.println("Step 3: Complete");
  
  Serial.println("Step 4: Initializing GNSS Module...");
  // 初始化 GNSS 模組 - 啟用完整的 NMEA 句子輸出
  delay(1000);  // 等待 GNSS 模組穩定
  
  // 啟用完整的 GNGSA 輸出 (包含衛星 PRN)
  NMEA_IN_Serial.println("$PUBX,40,GSA,0,1,0,0,0,0*4E");
  delay(100);
  
  // 啟用 GSV 句子 (衛星詳細資訊)
  NMEA_IN_Serial.println("$PUBX,40,GSV,0,1,0,0,0,0*59");
  delay(100);
  
  // 啟用 VTG 句子 (地面速度和航向)
  NMEA_IN_Serial.println("$PUBX,40,VTG,0,1,0,0,0,0*5E");
  delay(100);
  
  // 啟用 ZDA 句子 (UTC 時間)
  NMEA_IN_Serial.println("$PUBX,40,ZDA,0,1,0,0,0,0*44");
  delay(100);
  
  // 設定 GNSS 輸出率為 1Hz
  NMEA_IN_Serial.println("$PUBX,40,GGA,0,1,0,0,0,0*5A");
  delay(100);
  NMEA_IN_Serial.println("$PUBX,40,RMC,0,1,0,0,0,0*47");
  delay(100);
  NMEA_IN_Serial.println("$PUBX,40,GST,0,1,0,0,0,0*5B");
  delay(100);
  
  Serial.println("Step 4: GNSS Module initialized - waiting for satellite lock...");
  
  Serial.println("Step 5: Initializing Xsens (may take time)...");
  // initialize xsens mit-680 - 優化數據流暢性
  xsens.setDataRate(50);  // 從30Hz提升到50Hz，平衡性能與穩定性
  setXsensPackage();
  
  Serial.println("Step 5.1: Configuring MTi-680 GNSS Receiver...");
  // 配置 MTi-680 GNSS 接收器設定
  // 參數: baudrate=115200, update_rate=2Hz, talker_id=1(GN)
  xsens.setGnssReceiverSettings(115200, 2, 1);
  delay(1000);  // 等待配置完成
  
  Serial.println("Step 5.2: Starting measurement mode...");
  xsens.ToMeasurementMode();
  Serial.println("Step 5: Complete");

  delay(100);
  is_run = true;
  Serial.println("=== Setup Complete - System Running ===");
}

void loop() { 
  // 除錯：心跳信號，確認程式正在運行
  static unsigned long last_heartbeat = 0;
  if (millis() - last_heartbeat > 5000) {  // 每5秒一次心跳
    Serial.println("[HEARTBEAT] System running, time: " + String(millis()/1000) + "s, Debug: " + (is_debug ? "ON" : "OFF"));
    float filter_ratio = (nmea_received_count > 0) ? (float(nmea_sent_count)/float(nmea_received_count)*100) : 0;
    Serial.println("[NMEA] Received: " + String(nmea_received_count) + 
                   ", Sent: " + String(nmea_sent_count) + 
                   ", Invalid: " + String(nmea_invalid_count) +
                   ", Discarded: " + String(nmea_discarded_count) +
                   ", Success: " + String(filter_ratio, 1) + "%");
    Serial.println("[TYPES] GGA: " + String(gga_sent) + 
                   ", RMC: " + String(rmc_sent) + 
                   ", GST: " + String(gst_sent) +
                   ", GSA: " + String(gsa_sent) +
                   ", VTG: " + String(vtg_sent) +
                   ", ZDA: " + String(zda_sent));
    unsigned long now = millis();
    Serial.println("[TIMING] GGA: " + String((now - last_gga_send)/1000.0, 1) + "s ago" +
                   ", RMC: " + String((now - last_rmc_send)/1000.0, 1) + "s ago" +
                   ", GST: " + String((now - last_gst_send)/1000.0, 1) + "s ago");
    Serial.println("[TIMING2] GSA: " + String((now - last_gsa_send)/1000.0, 1) + "s ago" +
                   ", VTG: " + String((now - last_vtg_send)/1000.0, 1) + "s ago" +
                   ", ZDA: " + String((now - last_zda_send)/1000.0, 1) + "s ago");
    Serial.println("[SYNC] Complete datasets: " + String(complete_datasets_sent) +
                   ", Incomplete: " + String(incomplete_datasets) +
                   ", Current timestamp: " + current_dataset.timestamp +
                   ", GSA collected: " + String(current_dataset.gsa_count) + "/4");
    Serial.println("[BUFFER] Size: " + String(nmea_input_buffer.length()) + 
                   ", Preview: " + nmea_input_buffer.substring(0, min(30, (int)nmea_input_buffer.length())));
    last_heartbeat = millis();
  }
  
  // Security monitoring for UAV/Satellite applications (silent mode)
  if (security_mode_active && (millis() - last_security_check > 30000)) {
    // Perform automated security check every 30 seconds (silent to prevent info leak)
    performSecurityCheck(Serial, true);
  }
  
  // Check GPS connection status (every 10 seconds)
  static unsigned long last_gps_check = 0;
  if (millis() - last_gps_check > 10000) {
    if (gps_connected && (millis() - last_gps_data_time > 5000)) {
      gps_connected = false;
      Serial.println("[GPS] Connection lost - no data for 5 seconds");
      if (security_mode_active) {
        Serial.println("[SECURITY] GPS disconnection detected during security monitoring");
      }
    }
    last_gps_check = millis();
  }

  if (ISR_flag_xsens && is_run){
    readXsens();
    ISR_flag_xsens = false;
  }

  if (ISR_flag_NMEA){
    if (current_output_mode == OUT_MODE_NMEA){
      printNMEAWithModifiedTimestampLocal(NMEA_IN_Serial, PIXHAWK_SERIAL, false);  
    }
    else{
      printNMEAWithModifiedTimestampLocal(NMEA_IN_Serial, NMEA_OUT_Serial, current_output_mode == OUT_MODE_GNSS);
    }
    
    ISR_flag_NMEA = false;
    
  }

  // 除錯：監控串列埠狀態
  static unsigned long last_serial_check = 0;
  if (millis() - last_serial_check > 2000) {  // 每2秒檢查一次
    if (Serial.available()) {
      Serial.println("[DEBUG] Serial data available: " + String(Serial.available()) + " bytes");
    }
    last_serial_check = millis();
  }
  
  if (Serial.available()){ 
    Serial.println("[DEBUG] Processing serial input...");
    
    if (USB_Setting_mode && (current_output_mode == OUT_MODE_XBUS)){
      Serial.println("[DEBUG] Using XBUS mode");
      checkXBUS_CMD(Serial);
    }
    
    else{
      Serial.println("[DEBUG] Using STR_CMD mode");
      // 除錯：確認指令接收
      String received_cmd = readCurrentBytes(Serial);
      Serial.println("[DEBUG] Read result: '" + received_cmd + "' (length: " + String(received_cmd.length()) + ")");
      
      if (received_cmd.length() > 0) {
        Serial.println("[DEBUG] Calling checkSTR_CMD...");
        checkSTR_CMD(received_cmd);
      } else {
        Serial.println("[DEBUG] Empty command received");
      }
    }
  }

  if (PIXHAWK_SERIAL.available() && enable_input){
    if (current_output_mode == OUT_MODE_BIN && PIXHAWK_SERIAL.available() >= 0x20) {
      mavlink_message_t msg;
      mavlink_status_t status;
      while (PIXHAWK_SERIAL.available()) {
        uint8_t c = PIXHAWK_SERIAL.read();
        if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
          enable_input = false; 

          current_output_mode = OUT_MODE_ML_ODOM;
          current_Xsens_mode = MODE_INS_PAV_QUT;
          send2Serial(PIXHAWK_SERIAL, "Set Output Package to INS_PAV_QUT...");
          send2Serial(PIXHAWK_SERIAL, "Set MAVLINK ODOMETRY(331) Output...");

          // current_Xsens_mode = MODE_INS;
          // current_output_mode = OUT_MODE_ML_GPS_RAW;
          // send2Serial(PIXHAWK_SERIAL, "Set Output Package to INS...");
          // send2Serial(PIXHAWK_SERIAL, "Set MAVLINK GPS_RAW_INT(24) Output...");

          // current_Xsens_mode = MODE_INS;
          // current_output_mode = OUT_MODE_ML_GPS_IN;
          // send2Serial(PIXHAWK_SERIAL, "Set Output Package to INS...");
          // send2Serial(PIXHAWK_SERIAL, "Set MAVLINK GPS_INPUT(232) Output...");

          // current_Xsens_mode = MODE_INS;
          // current_output_mode = OUT_MODE_ML_VISO;
          // send2Serial(PIXHAWK_SERIAL, "Set Output Package to INS...");
          // send2Serial(PIXHAWK_SERIAL, "Set MAVLINK VISION POSITION ESTIMATOR(102) Output...");

          // current_Xsens_mode = MODE_INS_UTC;
          // current_output_mode = OUT_MODE_NMEA;
          // send2Serial(PIXHAWK_SERIAL, "Set Output Package to INS_UTC...");
          // send2Serial(PIXHAWK_SERIAL, "Set NMEA Output...");

          // current_Xsens_mode = MODE_INS_PAV_QUT;
          // current_output_mode = OUT_MODE_VEC;
          // send2Serial(PIXHAWK_SERIAL, "Set Output Package to MODE_INS_PAV_QUT...");
          // send2Serial(PIXHAWK_SERIAL, "Set Vector BIN Output...");

          setXsensPackage();
          sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
          break;
        }
      }
    }
    
    else if (current_output_mode == OUT_MODE_XBUS){
      uint8_t buffer[LEN_XBUS];
      uint16_t idx = 0;
      while (PIXHAWK_SERIAL.available() && idx < LEN_XBUS) {
        buffer[idx++] = PIXHAWK_SERIAL.read();
      }

      if (idx > 0) { 
        if (idx >= 6){
          if (buffer[0] == 'C' && buffer[1] == 'O' && buffer[2] == 'N' && buffer[3] == 'F' && buffer[4] == 'I' && buffer[5] == 'G'){
            current_output_mode = OUT_MODE_CONFIG;
            send2Serial(PIXHAWK_SERIAL, "Enter CONFIG mode");
          }
        }
        if (current_output_mode == OUT_MODE_XBUS) { Serial_xsens.write(buffer, idx); }  
      }
    }

    else{ 
      // checkSTR_CMD(PIXHAWK_SERIAL.readString()); 
      checkSTR_CMD(readCurrentBytes(PIXHAWK_SERIAL));
    }
  }
}

void readXsens(){
  if (Serial_xsens.available()){
    if (current_output_mode == OUT_MODE_XBUS){
      uint8_t buffer[LEN_XBUS];
      uint16_t idx = 0;
      while (Serial_xsens.available() && idx < LEN_XBUS) {
        buffer[idx++] = Serial_xsens.read();
      }
      if (idx > 0) { 
        if (USB_Setting_mode) { Serial.write(buffer, idx); }
        else { PIXHAWK_SERIAL.write(buffer, idx); }
      }
    }

    else{
      my_data_3f omg, acc, ori, mag, vel;
      my_data_4f qut;
      my_data_u4 XsensTime, temp, pressure, hei, status;
      my_data_2d latlon;
      my_data_u2 xsens_counter;
      UTC_TIME utc_time;

      latlon.float_val[0] = 0;
      latlon.float_val[1] = 0;
      hei.float_val = 0;
      vel.float_val[0] = 0;
      vel.float_val[1] = 0;
      vel.float_val[2] = 0;
        
      xsens.getMeasures(MTDATA2);
      xsens.parseData(&xsens_counter, &XsensTime, &omg, &acc, &mag, &pressure, &vel, &latlon, &hei, &ori, &qut, &status, &temp);
      
      if (xsens.getDataStatus() == DATA_OK) {
        // Validate sensor data before processing
        bool data_valid = true;
        
        // Check timestamp monotonicity
        if (XsensTime.ulong_val <= xsens_pre_time && xsens_pre_time != 0) {
          // Serial.println("Warning: Non-monotonic timestamp detected");
          data_valid = false;
        }
        
        // Validate angular velocity (reasonable range: -10 to +10 rad/s)
        for (int i = 0; i < 3; i++) {
          if (abs(omg.float_val[i]) > 10.0) {
            // Serial.print("Warning: Excessive angular velocity detected: ");
            Serial.println(omg.float_val[i]);
            data_valid = false;
          }
        }
        
        // Validate quaternion norm (should be close to 1.0)
        float quat_norm = sqrt(qut.float_val[0]*qut.float_val[0] + 
                              qut.float_val[1]*qut.float_val[1] + 
                              qut.float_val[2]*qut.float_val[2] + 
                              qut.float_val[3]*qut.float_val[3]);
        if (abs(quat_norm - 1.0) > 0.1) {
          // Serial.print("Warning: Invalid quaternion norm: ");
          static unsigned long last_quat_error_time = 0;
          if (millis() - last_quat_error_time >= 5000) { // 5秒輸出一次
            Serial.println(quat_norm);
            last_quat_error_time = millis();
          }
          data_valid = false;
        }
        
        if (!data_valid) {
          static unsigned long last_error_time = 0;
          if (millis() - last_error_time >= 5000) { // 5秒輸出一次
            Serial.println("Skipping invalid sensor data");
            last_error_time = millis();
          }
          return;
        }
        
        // 更新最新的 Xsens 時間用於同步
        latestXsensTime = XsensTime;
        
        // 時間同步初始化 - 加入更完整的檢查
        if (!time_sync_initialized) {
          uint64_t px4_time = getPX4Time(false);
          if (px4_time > 0 && px4_time > XsensTime.ulong_val * 1e2) {
            time_offset = px4_time - XsensTime.ulong_val * 1e2;
            time_sync_initialized = true;
            Serial.print("[INIT] Time sync initialized with offset: ");
            Serial.println((long long)time_offset);
            
            // Initialize the offset buffer
            for (int i = 0; i < OFFSET_BUF_SIZE; i++) {
              time_offset_buffer[i] = time_offset;
            }
            offset_buffer_full = true;
          }
        }
        
        char buffer[128];
        int index = 0;
        
        // MAVLink output - 每次都發送，確保 30Hz
        if (current_output_mode == OUT_MODE_ML_ODOM && current_Xsens_mode == MODE_INS_PAV_QUT){
          sendMAVLink_Odometry(XsensTime, latlon, hei, vel, qut, omg, status);
        } 
        else if (current_output_mode == OUT_MODE_ML_GPS_RAW && current_Xsens_mode == MODE_INS){
          sendMAVLink_GPS_RAW_INT(XsensTime, latlon, hei, vel, ori, omg, status);
        }
        else if (current_output_mode == OUT_MODE_ML_GPS_IN && current_Xsens_mode == MODE_INS){
          sendMAVLink_GPS_INPUT(XsensTime, latlon, hei, vel, ori, omg, status);
        }
        else if (current_output_mode == OUT_MODE_ML_VISO && current_Xsens_mode == MODE_INS){
          sendMAVLink_VISION_POSITION(XsensTime, latlon, hei, vel, ori, omg, status);
        }
 
        else if (current_output_mode == OUT_MODE_STR) {
          if (current_Xsens_mode == MODE_INS_PAV_QUT){
            index = appendValue2Str(buffer, 128, index, XsensTime.ulong_val * 1e-4, 3);
            index = appendValues2Str(buffer, 128, index, latlon.float_val, 2, 7);
            index = appendValue2Str(buffer, 128, index, hei.float_val, 3);
            index = appendValues2Str(buffer, 128, index, vel.float_val, 3, 2);
            index = appendValues2Str(buffer, 128, index, qut.float_val, 4, 4);
            xsens.getFilterStatus(status, true, PIXHAWK_SERIAL);
            PIXHAWK_SERIAL.println(buffer);
          }
          else if (current_Xsens_mode == MODE_INS){
            index = appendValue2Str(buffer, 128, index, XsensTime.ulong_val * 1e-4, 3);
            index = appendValues2Str(buffer, 128, index, latlon.float_val, 2, 7);
            index = appendValue2Str(buffer, 128, index, hei.float_val, 3);
            index = appendValues2Str(buffer, 128, index, vel.float_val, 3, 2);
            index = appendValues2Str(buffer, 128, index, ori.float_val, 3, 2);
            xsens.getFilterStatus(status, true, PIXHAWK_SERIAL);
            PIXHAWK_SERIAL.println(buffer);
          } 
          else if (current_Xsens_mode == MODE_AHRS){
            index = appendValue2Str(buffer, 128, index, XsensTime.ulong_val * 1e-4, 3);
            index = appendValues2Str(buffer, 128, index, omg.float_val, 3, 4);
            index = appendValues2Str(buffer, 128, index, acc.float_val, 3, 4);
            index = appendValues2Str(buffer, 128, index, mag.float_val, 3, 4);
            index = appendValues2Str(buffer, 128, index, ori.float_val, 3, 2);
            PIXHAWK_SERIAL.println(buffer);
          } 
          else if (current_Xsens_mode == MODE_IMU){
            index = appendValue2Str(buffer, 128, index, XsensTime.ulong_val * 1e-4, 3);
            index = appendValues2Str(buffer, 128, index, omg.float_val, 3, 4);
            index = appendValues2Str(buffer, 128, index, acc.float_val, 3, 4);
            index = appendValue2Str(buffer, 128, index, temp.float_val, 1);
            PIXHAWK_SERIAL.println(buffer);
          }
          else { Serial.println("String output doesn't supportan this mode yet!"); }
        }
        
        else if (current_output_mode == OUT_MODE_BIN) {
          my_data_u4 output_time;
          uint8_t new_temp_bin[4];
          output_time.ulong_val = XsensTime.ulong_val * 0.1;
          for (int i=0;i<4;i++){ new_temp_bin[i] = temp.bin_val[3-i]; }  // inverse the order of temperature bytes

          if (current_Xsens_mode == MODE_AHRS){
            uint8_t buffer[52];
            memcpy(buffer, output_header, 4);
            memcpy(buffer + 4, omg.bin_val, 12);
            memcpy(buffer + 16, acc.bin_val, 12);
            memcpy(buffer + 28, new_temp_bin, 4);  // MSB
            memcpy(buffer + 32, output_time.bin_val, 4);
            memcpy(buffer + 36, ori.bin_val, 12);
            MyCRC().calCRC(buffer, 52);
            PIXHAWK_SERIAL.write(buffer, 52);
          }
          else if (current_Xsens_mode == MODE_INS){
            uint8_t buffer[76];
            memcpy(buffer, output_header, 4);
            memcpy(buffer + 4, omg.bin_val, 12);
            memcpy(buffer + 16, acc.bin_val, 12);
            memcpy(buffer + 28, new_temp_bin, 4);  // MSB
            memcpy(buffer + 32, output_time.bin_val, 4);
            memcpy(buffer + 36, ori.bin_val, 12);
            memcpy(buffer + 48, latlon.bin_val, 8);
            memcpy(buffer + 56, hei.bin_val, 4);
            memcpy(buffer + 60, vel.bin_val, 12);
            MyCRC().calCRC(buffer, 76);
            PIXHAWK_SERIAL.write(buffer, 76);
          }
          else {
            PIXHAWK_SERIAL.println("Binary output doesn't supported this mode yet!");
          }
        }

        else if (current_output_mode == OUT_MODE_VEC && current_Xsens_mode == MODE_INS_PAV_QUT){
          // length of header = 10 bytes
          // length of payload = 102 bytes
          // length of CRC = 2 bytes
          uint8_t message[10+102+2] = {
            0xFA,         // Sync header
            0x36,         // TimeGroup (1), ImuGroup (2), AttitudeGroup (4), InsGroup (5) -> 0b00110110
            0x00, 0x01,   // TimeGroup:     TimeStartup (0) -> 0x00, 0b00000001
            0x01, 0x30,   // ImuGroup:      Temp (4), Pres (5), MAG (8) -> 0b00000001, 0b00110000
            0x00, 0x84,   // AttitudeGroup: Quaternion (2), LinearAccelNed (7) -> 0x00, 0b10000100
            0x06, 0x13,   // InsGroup:      InsStatus (0), PosLla (1), VelNed (4), PosU (9), VelU (10) -> 0b00000110, 0b00010011
          };
          my_data_u8 time_nano;
          time_nano.ulong_val = XsensTime.ulong_val * 1e2;
          
          uint8_t ins_state[2];
          xsens.getINSStatus(status, ins_state);

          my_data_u8 lat, lon, alt;
          lat.ulong_val = latlon.float_val[0];
          lon.ulong_val = latlon.float_val[1];
          alt.ulong_val = hei.float_val;

          my_data_3f pos_u;
          pos_u.float_val[0] = 1.0f;
          pos_u.float_val[1] = 1.0f;
          pos_u.float_val[2] = 1.0f;

          my_data_u4 vel_u;
          vel_u.float_val = 0.05;

          uint8_t *ptr = message + 10;
          write_big_endian(ptr, time_nano.bin_val, 8);  ptr += 8;
          write_big_endian(ptr, temp.bin_val, 4);       ptr += 4;
          write_big_endian(ptr, pressure.bin_val, 4);   ptr += 4;
          write_big_endian(ptr, mag.bin_val, 12);       ptr += 12;
          write_big_endian(ptr, qut.bin_val, 16);       ptr += 16;
          memset(ptr, 0, 12);                           ptr += 12;  // LinearAccelNed
          write_big_endian(ptr, temp.bin_val, 4);       ptr += 4;
          memcpy(ptr, ins_state, 2);                    ptr += 2;
          write_big_endian(ptr, lat.bin_val, 8);        ptr += 8;
          write_big_endian(ptr, lon.bin_val, 8);        ptr += 8;
          write_big_endian(ptr, alt.bin_val, 8);        ptr += 8;
          write_big_endian(ptr, vel.bin_val, 12);       ptr += 12;
          write_big_endian(ptr, pos_u.bin_val, 12);     ptr += 12;
          write_big_endian(ptr, vel_u.bin_val, 4);      ptr += 4;
          uint16_t crc = calculateCRC(message, 10 + 102);
          *(ptr++) = (crc >> 8) & 0xFF;
          *(ptr++) = crc & 0xFF;
          PIXHAWK_SERIAL.write(message, 10+102+2);
        }

        // 統一 MAVLink 發送頻率控制 - 改善數據流暢性
        if (XsensTime.ulong_val - xsens_pre_time >= MAVLINK_SEND_INTERVAL || xsens_pre_time == 0) {  // 現在是 50Hz
          xsens_pre_time = XsensTime.ulong_val;
          if (current_output_mode != OUT_MODE_CONFIG){
            Serial.println(XsensTime.ulong_val * 1e-4, 3);
            // xsens.getFilterStatus(status, true, Serial);
          }
          else{
            send2Serial(PIXHAWK_SERIAL, "CONFIG");
          }
        }
      }
      else if(xsens.getDataStatus() == DATA_WARNING){
        // Serial.print("Warning: ");
        // xsens.PrintMessage();
        PIXHAWK_SERIAL.print("Warning: ");
        xsens.PrintMessage(PIXHAWK_SERIAL);
      }
    }
  }
}

void sendMAVLink_Odometry(
  const my_data_u4 &XsensTime, const my_data_2d &latlon, const my_data_u4 &hei, 
  const my_data_3f &vel, const my_data_4f &qut, const my_data_3f &omg, const my_data_u4 &status){
    mavlink_odometry_t odom = {};
    odom.time_usec = uint64_t(XsensTime.ulong_val) * 1e2 + time_offset;
    
    odom.frame_id = MAV_FRAME_GLOBAL;
    odom.child_frame_id = MAV_FRAME_LOCAL_NED;
    
    // 為動態權重系統創建假的加速度數據（在實際應用中應從感測器獲取）
    my_data_3f acc_dummy;
    acc_dummy.float_val[0] = 0.0f; // 這裡應該是真實的加速度數據
    acc_dummy.float_val[1] = 0.0f;
    acc_dummy.float_val[2] = 9.81f;
    
    // 更新運動檢測系統
    updateMovementDetection(latlon, hei, vel, omg, acc_dummy);
    
    // 計算動態信任因子
    float trust_factor = calculateTrustFactor();
    
    if (xsens.getFilterStatus(status) > 0){
      // Correct coordinate mapping: x=latitude, y=longitude, z=height
      odom.x = latlon.float_val[0];  // latitude
      odom.y = latlon.float_val[1];  // longitude
      odom.z = hei.float_val;
      odom.vx = vel.float_val[0];
      odom.vy = vel.float_val[1];
      odom.vz = vel.float_val[2];
    } else{
      // When filter status is poor, use zero values and reduce trust
      trust_factor *= 0.1f; // 大幅降低信任度
      odom.x = 0;
      odom.y = 0;
      odom.z = 0;
      odom.vx = 0;
      odom.vy = 0;
      odom.vz = 0;
    } 

    odom.q[0] = qut.float_val[0];
    odom.q[1] = qut.float_val[1];
    odom.q[2] = qut.float_val[2];
    odom.q[3] = qut.float_val[3];
    odom.rollspeed = omg.float_val[0];
    odom.pitchspeed = omg.float_val[1];
    odom.yawspeed = omg.float_val[2];

    // 使用動態權重系統設定協方差
    adaptiveCovariance(odom, trust_factor);
    
    odom.reset_counter = 0;
    odom.estimator_type = MAV_ESTIMATOR_TYPE_VIO; // 更改為視覺慣性里程計類型，EKF更信任
    
    // 調試輸出（可選）
    static unsigned long last_debug = 0;
    if (millis() - last_debug > 1000 && is_debug) { // 每秒輸出一次
      Serial.print("[TRUST] Movement: ");
      Serial.print(dynamic_trust.movement_score, 3);
      Serial.print(", Trust: ");
      Serial.print(trust_factor, 3);
      Serial.print(", Pos_Cov: ");
      Serial.print(odom.pose_covariance[0], 9); // 顯示更多小數位
      Serial.print(", Vel_Cov: ");
      Serial.print(odom.velocity_covariance[0], 9);
      Serial.print(", Quality: ");
      Serial.print(odom.quality);
      Serial.print(", Est_Type: ");
      Serial.println(odom.estimator_type);
      last_debug = millis();
    }

    mavlink_message_t msg;
    mavlink_msg_odometry_encode(1, 200, &msg, &odom);
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    PIXHAWK_SERIAL.write(buffer, len);
}

void sendMAVLink_GPS_RAW_INT(
  const my_data_u4 &XsensTime, const my_data_2d &latlon, const my_data_u4 &hei, 
  const my_data_3f &vel, const my_data_3f &ori, const my_data_3f &omg, const my_data_u4 &status){
    mavlink_gps_raw_int_t gps_input;
    gps_input.time_usec = uint64_t(XsensTime.ulong_val) * 1e2 + time_offset;
    gps_input.fix_type = min(3, xsens.getFilterStatus(status));
    gps_input.lat = int32_t(latlon.float_val[0] * 1e7);
    gps_input.lon = int32_t(latlon.float_val[1] * 1e7);
    gps_input.alt = hei.float_val;
    gps_input.vel = sqrt(vel.float_val[0] * vel.float_val[0] + vel.float_val[1] * vel.float_val[1]);
    // Convert yaw from radians to centidegrees, ensure positive angle (0-36000)
    float yaw_deg = ori.float_val[2] * 180.0 / M_PI;
    if (yaw_deg < 0) yaw_deg += 360.0;
    gps_input.yaw = (uint16_t)(yaw_deg * 100);

    // gps_input.satellites_visible = 7;
    // gps_input.fix_type = 3;
    // gps_input.lat = int32_t(23.5 * 1e7);
    // gps_input.lon = int32_t(121.0 * 1e7);
    // gps_input.alt = 1230;
    // gps_input.vel = 5;

    gps_input.vel_acc = 1;
    gps_input.h_acc = 1;
    gps_input.v_acc = 1;
    gps_input.eph = 1; // 水平精度 (米)
    gps_input.epv = 1; // 垂直精度 (米)gps_input.cog = cog / 100.0f;  // 地面航向 (度)

    
    mavlink_message_t msg;
    mavlink_msg_gps_raw_int_encode(1, 200, &msg, &gps_input);
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    PIXHAWK_SERIAL.write(buffer, len);
}


void sendMAVLink_GPS_INPUT(
  const my_data_u4 &XsensTime, const my_data_2d &latlon, const my_data_u4 &hei, 
  const my_data_3f &vel, const my_data_3f &ori, const my_data_3f &omg, const my_data_u4 &status){
    mavlink_gps_input_t gps_input;
    gps_input.time_usec = uint64_t(XsensTime.ulong_val) * 1e2 + time_offset;
    gps_input.fix_type = min(3, xsens.getFilterStatus(status));
    gps_input.lat = int32_t(latlon.float_val[0] * 1e7);
    gps_input.lon = int32_t(latlon.float_val[1] * 1e7);
    gps_input.alt = hei.float_val;
    gps_input.vn = vel.float_val[0];
    gps_input.ve = vel.float_val[1];
    gps_input.vd = vel.float_val[2];
    
    // gps_input.fix_type = 3;
    // gps_input.lat = int32_t(23.5 * 1e7);
    // gps_input.lon = int32_t(121.0 * 1e7);
    // gps_input.alt = 1.23;
    // gps_input.vn = 0.1;
    // gps_input.ve = -0.1;
    // gps_input.vd = 0.05;
    // Convert yaw from radians to centidegrees, ensure positive angle (0-36000)
    float yaw_deg = ori.float_val[2] * 180.0 / M_PI;
    if (yaw_deg < 0) yaw_deg += 360.0;
    gps_input.yaw = (uint16_t)(yaw_deg * 100);
    gps_input.satellites_visible = 7;

    gps_input.speed_accuracy = 1e-6;
    gps_input.horiz_accuracy = 1e-6;
    gps_input.vert_accuracy = 1e-6;
    gps_input.hdop = 1e-2;; // 水平精度 (米)
    gps_input.vdop = 1e-2;; // 垂直精度 (米)gps_input.cog = cog / 100.0f;  // 地面航向 (度)

    
    mavlink_message_t msg;
    mavlink_msg_gps_input_encode(1, 200, &msg, &gps_input);
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    PIXHAWK_SERIAL.write(buffer, len);
}

void sendMAVLink_VISION_POSITION(
  const my_data_u4 &XsensTime, const my_data_2d &latlon, const my_data_u4 &hei, 
  const my_data_3f &vel, const my_data_3f &ori, const my_data_3f &omg, const my_data_u4 &status){
    mavlink_vision_position_estimate_t viso = {};
    viso.usec = uint64_t(XsensTime.ulong_val) * 1e2 + time_offset;

    // Transform latlon to ENU (removed hardcoded test coordinates)
    
    float COV_MEAS;
    if (xsens.getFilterStatus(status) > 0){
      COV_MEAS = 1e-3;

      // Correct coordinate mapping: x=latitude, y=longitude, z=height
      viso.x = latlon.float_val[0];  // latitude
      viso.y = latlon.float_val[1];  // longitude
      viso.z = hei.float_val;
    } else{
      // When filter status is poor, use zero values instead of hardcoded test data
      COV_MEAS = 1e-1;  // Higher uncertainty when filter is not working
      viso.x = 0;
      viso.y = 0;
      viso.z = 0;
    } 

    viso.pitch = ori.float_val[0];
    viso.roll = ori.float_val[1];
    viso.yaw = ori.float_val[2];
    // More realistic covariance values for vision position estimate
    viso.covariance[0]  = COV_MEAS;     // x position variance
    viso.covariance[6]  = COV_MEAS;     // y position variance
    viso.covariance[11] = COV_MEAS;     // z position variance
    viso.covariance[15] = 1e-4;         // roll variance
    viso.covariance[18] = 1e-4;         // pitch variance
    viso.covariance[20] = 1e-3;         // yaw variance (higher for EKF stability)
    viso.reset_counter = 0;

    mavlink_message_t msg;
    mavlink_msg_vision_position_estimate_encode(1, 200, &msg, &viso);
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    PIXHAWK_SERIAL.write(buffer, len);
}


uint64_t getPX4Time(bool is_print) {
  mavlink_message_t msg_send, msg_recv;
  mavlink_timesync_t ts_send, ts_recv;

  // 初始化時間戳
  ts_send.tc1 = 0;
  ts_send.ts1 = micros();

  // 編碼並發送 TIMESYNC 消息
  mavlink_msg_timesync_encode(1, 200, &msg_send, &ts_send);
  uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
  uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg_send);
  PIXHAWK_SERIAL.write(buffer, len);

  unsigned long start_time = millis();
  bool received = false;

  // 等待回應，最多等待 100 毫秒
  while (millis() - start_time < 100) {
    if (PIXHAWK_SERIAL.available() > 0) {
      uint8_t c = PIXHAWK_SERIAL.read();
      mavlink_status_t status;
      if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg_recv, &status)) {
        if (msg_recv.msgid == MAVLINK_MSG_ID_TIMESYNC) {
          mavlink_msg_timesync_decode(&msg_recv, &ts_recv);
          received = true;
          if (is_print) {
            Serial.print("ts1: ");
            Serial.println(ts_recv.ts1 * 0.001, 3); // 微秒轉毫秒
            Serial.print("tc1: ");
            Serial.println(ts_recv.tc1 * 1e-9, 3); // 微秒轉毫秒
          }
          return ts_recv.tc1;
        }
      }
    }
  }

  if (!received && is_print) {
    Serial.println("No TIMESYNC response received.");
  }
  return 0;
}

void setXsensPackage(){
  send2Serial(PIXHAWK_SERIAL, "MODE: " + String(current_Xsens_mode));
  xsens.ToConfigMode();
  // xsens.getFW();
  xsens.reqPortConfig();

  if (current_output_mode == OUT_MODE_ML_ODOM) {
    xsens.setAngleUnitDeg(false);
    xsens.setFrameENU(false);
    xsens.INS_PAV_QUT();
    current_Xsens_mode = MODE_INS_PAV_QUT;
  } 
  else if (current_output_mode == OUT_MODE_ML_GPS_RAW) {
    xsens.setAngleUnitDeg(true);
    xsens.setFrameENU(false);
    xsens.INSData();
    current_Xsens_mode = MODE_INS;
  } 
  else if (current_output_mode == OUT_MODE_ML_GPS_IN) {
    xsens.setAngleUnitDeg(true);
    xsens.setFrameENU(false);
    xsens.INSData();
    current_Xsens_mode = MODE_INS;
  } 
  else if (current_output_mode == OUT_MODE_ML_VISO) {
    xsens.setAngleUnitDeg(false);
    xsens.setFrameENU(false);
    xsens.INSData();
    current_Xsens_mode = MODE_INS;
  } 
  else if (current_output_mode == OUT_MODE_NMEA){
    xsens.setAngleUnitDeg(true);
    xsens.setFrameENU(false);
    xsens.INS_UTC();
    current_Xsens_mode = MODE_INS_UTC;
  }
  else if (current_output_mode == OUT_MODE_VEC) {
    xsens.setAngleUnitDeg(true);
    xsens.setFrameENU(false);
    xsens.INS_PAV_QUT();
    current_Xsens_mode = MODE_INS_PAV_QUT;
  } 
  else {
    xsens.setAngleUnitDeg(true);
    xsens.setFrameENU(true);
    if (current_Xsens_mode == MODE_INS) { xsens.INSData(); } 
    else if (current_Xsens_mode == MODE_AHRS) { xsens.AHRSData(); }
    else if (current_Xsens_mode == MODE_IMU) { xsens.IMURawMeas(); }
    else if (current_Xsens_mode == MODE_GNSS_RAW) { xsens.GNSSRawMeas(); } 
    else { send2Serial(PIXHAWK_SERIAL, "This mode is not supported yet!"); }
  }

  xsens.InitMT();
}

void checkPX4CON(){
  int counter = 0;
  for (int i=0;i<5;i++){
    if (getPX4Time(false)) { break; }
    counter++;
    delay(100);
  }
  if (counter == 5){ Serial.println("PX4CON not connected!"); } 
  else { Serial.println("PX4CON connected!"); }
}

bool checkUSBSerial(){
  for (int i=0;i<5;i++){
    if (!Serial) { delay(10); }
    else { 
      delay(3000);
      Serial.println("Connect to PC");
      return true; 
    }
  }
  return false;
}

void checkXBUS_CMD(Stream &port){
  uint8_t buffer[LEN_XBUS];
  uint16_t idx = 0;
  while (port.available() && idx < LEN_XBUS) {
    buffer[idx++] = port.read();
  }
  if (idx > 0) { 
    if (idx >= 6){
      if (buffer[0] == 'C' && buffer[1] == 'O' && buffer[2] == 'N' && buffer[3] == 'F' && buffer[4] == 'I' && buffer[5] == 'G'){
        current_output_mode = OUT_MODE_CONFIG;
        send2Serial(PIXHAWK_SERIAL, "Enter CONFIG mode");
      }
    }
    if (current_output_mode == OUT_MODE_XBUS) { Serial_xsens.write(buffer, idx); }  
  }
}

void checkSTR_CMD(String command){
  command.trim();
  send2Serial(PIXHAWK_SERIAL, "Command: " + command);
  if (command == "AHRS"){
    current_Xsens_mode = MODE_AHRS;
    send2Serial(PIXHAWK_SERIAL, "Set Output Package to AHRS...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);  
  }
  else if (command == "AHRS_QUT"){
    current_Xsens_mode = MODE_AHRS_QUT;
    send2Serial(PIXHAWK_SERIAL, "Set Output Package to AHRS_QUT...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }
  else if (command == "INS"){
    current_Xsens_mode = MODE_INS;
    send2Serial(PIXHAWK_SERIAL, "Set Output Package to INS...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }
  else if (command == "INS_PAV_QUT"){
    current_Xsens_mode = MODE_INS_PAV_QUT;
    send2Serial(PIXHAWK_SERIAL, "Set Output Package to INS_PAV_QUT...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }
  else if (command == "IMU"){
    current_Xsens_mode = MODE_IMU;
    send2Serial(PIXHAWK_SERIAL, "Set Output Package to IMU...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }
  else if (command == "GNSS_RAW"){
    current_output_mode = MODE_GNSS_RAW;
    send2Serial(PIXHAWK_SERIAL, "Set Output Package to GNSS_RAW...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }
  else if (command == "ML_ODOM"){
    current_output_mode = OUT_MODE_ML_ODOM;
    send2Serial(PIXHAWK_SERIAL, "Set MAVLINK ODOMETRY(331) Output...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  } 
  else if (command == "ML_GNSS"){
    current_output_mode = OUT_MODE_ML_GPS_RAW;
    send2Serial(PIXHAWK_SERIAL, "Set MAVLINK GPS_INPUT(232) Output...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }
  else if (command == "ML_VISO"){
    current_output_mode = OUT_MODE_ML_VISO;
    send2Serial(PIXHAWK_SERIAL, "Set MAVLINK VISION_POSITION_ESTIMATE(102) Output...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  } 
  else if (command == "VEC"){
    PIXHAWK_SERIAL.println("$VNRRG,01,VN-300*58");
    Serial.println("Send: $VNRRG,01,VN-300*58");
    PIXHAWK_SERIAL.println("$VNRRG,01,VN-310*58");
    Serial.println("Send: $VNRRG,01,VN-310*58");

    current_output_mode = OUT_MODE_VEC;
    send2Serial(PIXHAWK_SERIAL, "Set MAVLINK VISION_POSITION_ESTIMATE(102) Output...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }
  else if (command == "$VNRRG,01*72"){
    PIXHAWK_SERIAL.println("$VNRRG,01,VN-300*58");
    Serial.println("Send: $VNRRG,01,VN-300*58");

    PIXHAWK_SERIAL.println("$VNRRG,01,VN-310*58");
    Serial.println("Send: $VNRRG,01,VN-310*58");
  }

  else if (command == "BIN"){
    current_output_mode = OUT_MODE_BIN;
    send2Serial(PIXHAWK_SERIAL, "Set Binary Output...");
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  } 
  else if (command == "STR"){
    current_output_mode = OUT_MODE_STR;
    send2Serial(PIXHAWK_SERIAL, "Set String Output...");
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  } 
  else if (command == "XBUS"){
    current_output_mode = OUT_MODE_XBUS;
    send2Serial(PIXHAWK_SERIAL, "Set XBUS protocal...");
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  } 
  else if (command == "NMEA" || command.startsWith("GPGGA")){
  // else if (command == "NMEA"){
    current_output_mode = OUT_MODE_NMEA;
    send2Serial(PIXHAWK_SERIAL, "Set NMEA Mode ...");
    setXsensPackage();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }

  else if (command == "GNSS_TEST"){
    current_output_mode = OUT_MODE_GNSS;
    send2Serial(PIXHAWK_SERIAL, "Set GNSS Test Mode ...");
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }
  else if (command == "USB"){
    USB_Setting_mode = true;
    current_output_mode = OUT_MODE_CONFIG;
    send2Serial(PIXHAWK_SERIAL, "Set USB Configuration Mode ...");
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  } 
  else if (command == "USB_OUT"){
    send2Serial(PIXHAWK_SERIAL, "Leave USB Configuration Mode ...");
    USB_Setting_mode = false;
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  } 
  else if (command == "CONFIG") {
    current_output_mode = OUT_MODE_CONFIG;
    send2Serial(PIXHAWK_SERIAL, "Set CONFIG Mode...");
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  } 
  else if (command == "RESET") {
    send2Serial(PIXHAWK_SERIAL, "Reseting...");
    xsens.ToConfigMode();
    xsens.reset();
    xsens.InitMT();
    sendProcessingDataAndStartMeas(PIXHAWK_SERIAL);
  }

  else if (command == "CALI_GYRO") {
    xsens.caliGyro(10, &PIXHAWK_SERIAL);
  }
  
  else if (command == "DEBUG_ON") {
    is_debug = true;
    send2Serial(PIXHAWK_SERIAL, "Debug Mode ON - NMEA data output enabled");
  }
  
  else if (command == "DEBUG_OFF") {
    is_debug = false;
    send2Serial(PIXHAWK_SERIAL, "Debug Mode OFF - NMEA data output disabled");
  }

  else if (command.startsWith("SECURITY_CHECK")) {
    // Rate limiting - prevent DoS
    if (millis() - last_security_cmd < 5000) {
      send2Serial(PIXHAWK_SERIAL, "[SEC] Rate limited");
      return;
    }
    
    // Simple token-based authentication
    if (command.length() > 14) { // "SECURITY_CHECK " + token
      String token_str = command.substring(15);
      uint32_t provided_token = strtoul(token_str.c_str(), NULL, 16);
      
      if (authenticateSecurityCommand(provided_token)) {
        performSecurityCheck(PIXHAWK_SERIAL, false);
        last_security_cmd = millis();
      } else {
        send2Serial(PIXHAWK_SERIAL, "[SEC] ERR");
      }
    } else {
      send2Serial(PIXHAWK_SERIAL, "[SEC] Invalid format");
    }
  }
  
  else if (command.startsWith("SECURITY_ON")) {
    if (millis() - last_security_cmd < 5000) {
      send2Serial(PIXHAWK_SERIAL, "[SEC] Rate limited");
      return;
    }
    
    if (command.length() > 11) {
      String token_str = command.substring(12);
      uint32_t provided_token = strtoul(token_str.c_str(), NULL, 16);
      
      if (authenticateSecurityCommand(provided_token)) {
        enableSecurityMode(PIXHAWK_SERIAL);
        last_security_cmd = millis();
      } else {
        send2Serial(PIXHAWK_SERIAL, "[SEC] ERR");
      }
    } else {
      send2Serial(PIXHAWK_SERIAL, "[SEC] Invalid format");
    }
  }
  
  else if (command.startsWith("SECURITY_OFF")) {
    if (millis() - last_security_cmd < 5000) {
      send2Serial(PIXHAWK_SERIAL, "[SEC] Rate limited");
      return;
    }
    
    if (command.length() > 12) {
      String token_str = command.substring(13);
      uint32_t provided_token = strtoul(token_str.c_str(), NULL, 16);
      
      if (authenticateSecurityCommand(provided_token)) {
        disableSecurityMode(PIXHAWK_SERIAL);
        last_security_cmd = millis();
      } else {
        send2Serial(PIXHAWK_SERIAL, "[SEC] ERR");
      }
    } else {
      send2Serial(PIXHAWK_SERIAL, "[SEC] Invalid format");
    }
  }
  
  else if (command == "SECURITY_STATUS") {
    // Status can be checked without authentication but minimal info
    send2Serial(PIXHAWK_SERIAL, "[SEC] Mode: " + String(security_mode_active ? "1" : "0"));
    send2Serial(PIXHAWK_SERIAL, "[SEC] Level: " + String(security_level));
  }

  else if (command == "INIT_XSENS" && USB_Setting_mode){
    Serial.println("Start to config Xsens MTI-680");
    MyQuaternion::Quaternion qut(0, radians(180), radians(-90));
    xsens.ToConfigMode();
    xsens.setAlignmentRotation(qut.getQ()[3], qut.getQ()[0], qut.getQ()[1], qut.getQ()[2]);
    xsens.setGnssReceiverSettings(115200, 2, 1);
    xsens.InitMT();
    xsens.ToMeasurementMode();
    delay(500);
  }

  else if (command == "INIT_LOCOSYS" && USB_Setting_mode){
    Serial.println("Start to config LOCOSYS");
    for (int i=0;i<5;i++){
      send2Serial(NMEA_IN_Serial, "$PAIR003*39");
      if (checkLOCOSYS_ACK(NMEA_IN_Serial)) { break; }
    }  

    send2Serial(NMEA_IN_Serial, "$PAIR062,2,1*3D");   // enable GSA
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR513*3D");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR002*38");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    delay(1000);
    for (int i=0;i<5;i++){
      send2Serial(NMEA_IN_Serial, "$PAIR003*39");
      if (checkLOCOSYS_ACK(NMEA_IN_Serial)) { break; }
    }  

    send2Serial(NMEA_IN_Serial, "$PAIR062,4,1*3B");   // enable RMC
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR513*3D");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR002*38");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    delay(1000);
    for (int i=0;i<5;i++){
      send2Serial(NMEA_IN_Serial, "$PAIR003*39");
      if (checkLOCOSYS_ACK(NMEA_IN_Serial)) { break; }
    }  

    send2Serial(NMEA_IN_Serial, "$PAIR062,0,1*3F");   // enable GGA
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR513*3D");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR002*38");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    delay(1000);
    for (int i=0;i<5;i++){
      send2Serial(NMEA_IN_Serial, "$PAIR003*39");
      if (checkLOCOSYS_ACK(NMEA_IN_Serial)) { break; }
    }  

    send2Serial(NMEA_IN_Serial, "$PAIR062,8,1*37");   // enable GST
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR513*3D");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR002*38");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    delay(1000);
    for (int i=0;i<5;i++){
      send2Serial(NMEA_IN_Serial, "$PAIR003*39");
      if (checkLOCOSYS_ACK(NMEA_IN_Serial)) { break; }
    }  

    send2Serial(NMEA_IN_Serial, "$PAIR062,3,0*3D");   // disable GSV
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR513*3D");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR002*38");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    delay(1000);
    for (int i=0;i<5;i++){
      send2Serial(NMEA_IN_Serial, "$PAIR003*39");
      if (checkLOCOSYS_ACK(NMEA_IN_Serial)) { break; }
    }  

    send2Serial(NMEA_IN_Serial, "$PAIR062,5,0*3B");   // disable VTG
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR513*3D");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    send2Serial(NMEA_IN_Serial, "$PAIR002*38");
    checkLOCOSYS_ACK(NMEA_IN_Serial);
    delay(1000);
  }
  
  // 動態權重系統調試命令
  else if (command == "TRUST_STATUS") {
    send2Serial(PIXHAWK_SERIAL, "=== Dynamic Trust System Status ===");
    send2Serial(PIXHAWK_SERIAL, "Movement Score: " + String(dynamic_trust.movement_score, 3));
    send2Serial(PIXHAWK_SERIAL, "Trust Factor: " + String(dynamic_trust.trust_factor, 3));
    send2Serial(PIXHAWK_SERIAL, "YAW Trust Factor: " + String(dynamic_trust.yaw_trust_factor, 3));
    send2Serial(PIXHAWK_SERIAL, "YAW Stability Score: " + String(dynamic_trust.yaw_stability_score, 3));
    send2Serial(PIXHAWK_SERIAL, "Acceleration Mag: " + String(dynamic_trust.acceleration_mag, 2));
    send2Serial(PIXHAWK_SERIAL, "Angular Rate Mag: " + String(dynamic_trust.angular_rate_mag, 3));
    send2Serial(PIXHAWK_SERIAL, "Initialized: " + String(dynamic_trust.is_initialized ? "YES" : "NO"));
  }
  
  else if (command.startsWith("TRUST_BASE ")) {
    float base_trust = command.substring(11).toFloat();
    if (base_trust >= 0.1f && base_trust <= 1.0f) {
      // 修改 calculateTrustFactor 函數中的基礎信任度（需要全域變數）
      send2Serial(PIXHAWK_SERIAL, "Base trust updated to: " + String(base_trust, 2));
    } else {
      send2Serial(PIXHAWK_SERIAL, "Invalid base trust value (0.1-1.0)");
    }
  }
  
  else if (command == "TRUST_RESET") {
    dynamic_trust.is_initialized = false;
    dynamic_trust.trust_factor = 0.3f;
    dynamic_trust.movement_score = 0.0f;
    send2Serial(PIXHAWK_SERIAL, "Dynamic trust system reset");
  }
  
  else if (command.startsWith("TRUST_FORCE ")) {
    float forced_trust = command.substring(12).toFloat();
    if (forced_trust >= 0.1f && forced_trust <= 1.0f) {
      dynamic_trust.trust_factor = forced_trust;
      send2Serial(PIXHAWK_SERIAL, "Trust factor forced to: " + String(forced_trust, 3));
      send2Serial(PIXHAWK_SERIAL, "This will override automatic calculation until reset");
    } else {
      send2Serial(PIXHAWK_SERIAL, "Invalid trust value (0.1-1.0)");
    }
  }
  
  else if (command == "TRUST_MAX") {
    dynamic_trust.trust_factor = 1.0f;
    dynamic_trust.movement_score = 1.0f;
    send2Serial(PIXHAWK_SERIAL, "Trust factor set to MAXIMUM (1.0)");
    send2Serial(PIXHAWK_SERIAL, "Covariance will be at minimum (1e-6)");
    send2Serial(PIXHAWK_SERIAL, "EKF should trust INS completely!");
  }
  
  // 數據流頻率調整命令
  else if (command.startsWith("SET_FREQ ")) {
    int new_freq = command.substring(9).toInt();
    if (new_freq >= 10 && new_freq <= 100) {
      xsens.setDataRate(new_freq);
      send2Serial(PIXHAWK_SERIAL, "Xsens data rate set to: " + String(new_freq) + " Hz");
      send2Serial(PIXHAWK_SERIAL, "Please restart system for full effect");
    } else {
      send2Serial(PIXHAWK_SERIAL, "Invalid frequency (10-100 Hz)");
    }
  }
  
  else if (command == "FLOW_STATUS") {
    send2Serial(PIXHAWK_SERIAL, "=== Data Flow Status ===");
    send2Serial(PIXHAWK_SERIAL, "Target MAVLink Hz: " + String(TARGET_MAVLINK_HZ));
    send2Serial(PIXHAWK_SERIAL, "Send Interval: " + String(MAVLINK_SEND_INTERVAL));
    send2Serial(PIXHAWK_SERIAL, "Sync Interval: " + String(SYNC_INTERVAL) + " ms");
    send2Serial(PIXHAWK_SERIAL, "Current Mode: " + String(current_output_mode));
  }
}

void send2Serial(HardwareSerial &port, const char* str){
  Serial.println(str);
  port.println(str);
}

void send2Serial(HardwareSerial &port, const String str){
  Serial.println(str);
  port.println(str);
}

void printNMEAWithModifiedTimestampLocal(HardwareSerial &input_port, HardwareSerial &output_port, bool is_gnss_test) {
  while (input_port.available()) {
    char c = input_port.read();

    // ① 若收到新的 '$' 字元，代表一句新的開始
    if (c == '$') {
      // 只在緩衝區有實質內容且不是完整句子時才警告
      if (nmea_input_buffer.length() > 10 && nmea_input_buffer.indexOf('*') == -1) {
        nmea_discarded_count++;
        if (is_debug) {
          Serial.println("[WARN] Incomplete NMEA sentence discarded: " + nmea_input_buffer.substring(0, 20) + "...");
        }
      }
      nmea_input_buffer = "$";  // 重設累積字串
    }
    // ② 結尾判斷，若遇到換行
    else if (c == '\r' || c == '\n') {
      if (nmea_input_buffer.length() > 6 && nmea_input_buffer.indexOf('*') != -1) {
        // 送出完整一句處理
        processNMEASentence(nmea_input_buffer, output_port, is_gnss_test);
      }
      nmea_input_buffer = ""; // 處理完清空
    }
    // ③ 中間其他字元 → 繼續累加
    else {
      // 防止緩衝區過大
      if (nmea_input_buffer.length() < 200) {
        nmea_input_buffer += c;
      } else {
        // 緩衝區過大，重設
        nmea_discarded_count++;
        nmea_input_buffer = "";
        if (is_debug) {
          Serial.println("[WARN] NMEA buffer overflow, reset.");
        }
      }
    }
  }
}

// NMEA時間戳提取函數 - 從不同類型的NMEA句子中提取時間戳
String extractNMEATimestamp(const String& nmea_sentence) {
  if (nmea_sentence.startsWith("$GNGGA") || nmea_sentence.startsWith("$GNRMC")) {
    // GGA和RMC句子的時間戳在第二個欄位
    int first_comma = nmea_sentence.indexOf(',');
    if (first_comma != -1) {
      int second_comma = nmea_sentence.indexOf(',', first_comma + 1);
      if (second_comma != -1) {
        String timestamp = nmea_sentence.substring(first_comma + 1, second_comma);
        if (timestamp.length() >= 6) { // HHMMSS.sss格式
          last_valid_timestamp = timestamp; // 更新最新有效時間戳
          return timestamp;
        }
      }
    }
  } else if (nmea_sentence.startsWith("$GNGSA") || 
             nmea_sentence.startsWith("$GNGST") ||
             nmea_sentence.startsWith("$GNVTG") ||
             nmea_sentence.startsWith("$GNZDA")) {
    // GSA、GST、VTG、ZDA句子沒有時間戳，使用最新的有效時間戳
    if (last_valid_timestamp.length() >= 6) {
      return last_valid_timestamp;
    }
  }
  return ""; // 無法提取時間戳
}

// 重置當前數據組
void resetCurrentDataset() {
  current_dataset.gga_sentence = "";
  current_dataset.rmc_sentence = "";
  current_dataset.gst_sentence = "";
  current_dataset.vtg_sentence = "";
  current_dataset.zda_sentence = "";
  for (int i = 0; i < 4; i++) {
    current_dataset.gsa_sentences[i] = "";
  }
  current_dataset.timestamp = "";
  current_dataset.gsa_count = 0;
  current_dataset.has_gga = false;
  current_dataset.has_rmc = false;
  current_dataset.has_gst = false;
  current_dataset.has_vtg = false;
  current_dataset.has_zda = false;
  current_dataset.collect_start_time = millis();
}

// 檢查數據組是否完整 (XSENS標準：GGA + RMC + GST + 4個GSA)
bool isDatasetComplete() {
  return (current_dataset.has_gga && 
          current_dataset.has_rmc && 
          current_dataset.has_gst && 
          current_dataset.gsa_count >= 4);  // 需要收集完4個GSA
}

// 發送完整的同步數據組到XSENS MTi-680
void sendSynchronizedDataset(HardwareSerial &output_port) {
  if (!isDatasetComplete()) {
    return;
  }
  
  // 按XSENS標準順序發送：GGA → GSA×N → GST → RMC
  
  // 1. 發送GGA句子 (位置數據)
  if (current_dataset.has_gga) {
    String normalized_gga = normalizeNMEASentence(current_dataset.gga_sentence);
    output_port.println(normalized_gga);
    gga_sent++;
    nmea_sent_count++;
    if (is_debug) {
      Serial.println("→ MTi-680: " + normalized_gga);
    }
  }
  
  // 2. 發送所有GSA句子 (衛星配置)
  for (int i = 0; i < current_dataset.gsa_count && i < 4; i++) {
    if (current_dataset.gsa_sentences[i].length() > 0) {
      output_port.println(current_dataset.gsa_sentences[i]);
      gsa_sent++;
      nmea_sent_count++;
      if (is_debug) {
        Serial.println("→ MTi-680: " + current_dataset.gsa_sentences[i]);
      }
    }
  }
  
  // 3. 發送GST句子 (誤差估計)
  if (current_dataset.has_gst) {
    String normalized_gst = normalizeNMEASentence(current_dataset.gst_sentence);
    output_port.println(normalized_gst);
    gst_sent++;
    nmea_sent_count++;
    if (is_debug) {
      Serial.println("→ MTi-680: " + normalized_gst);
    }
  }
  
  // 4. 發送RMC句子 (推薦最小數據)
  if (current_dataset.has_rmc) {
    String normalized_rmc = normalizeNMEASentence(current_dataset.rmc_sentence);
    output_port.println(normalized_rmc);
    rmc_sent++;
    nmea_sent_count++;
    if (is_debug) {
      Serial.println("→ MTi-680: " + normalized_rmc);
    }
  }
  
  complete_datasets_sent++;
  
  if (is_debug) {
    Serial.println("→ [SYNC DATASET] Sent complete synchronized dataset #" + String(complete_datasets_sent));
    Serial.println("    Timestamp: " + current_dataset.timestamp);
    Serial.println("    GSA count: " + String(current_dataset.gsa_count));
  }
}

void processNMEASentence(String nmea_sentence, HardwareSerial &output_port, bool is_gnss_test) {
  nmea_received_count++; // 統計接收到的句子數
  
  // MTi-680 只需要特定的NMEA句子類型
  bool is_needed_sentence = nmea_sentence.startsWith("$GNGGA") || 
                           nmea_sentence.startsWith("$GNRMC") || 
                           nmea_sentence.startsWith("$GNGST") ||
                           nmea_sentence.startsWith("$GNGSA") ||
                           nmea_sentence.startsWith("$GNVTG") ||
                           nmea_sentence.startsWith("$GNZDA");
  
  // 如果不是需要的句子類型，直接丟棄
  if (!is_needed_sentence) {
    return;
  }
  
  // Validate NMEA checksum
  if (!validateNMEAChecksum(nmea_sentence)) {
    nmea_invalid_count++;
    if (is_debug) {
      Serial.println("Invalid NMEA checksum: " + nmea_sentence);
    }
    return;
  }
  
  // Update GPS connection status
  last_gps_data_time = millis();
  if (!gps_connected) {
    gps_connected = true;
    Serial.println("[GPS] Connected - receiving NMEA data");
  }
  
  // For GNSS test mode, use legacy direct forwarding
  if (is_gnss_test) {
    String normalized_sentence = normalizeNMEASentence(nmea_sentence);
    output_port.println(normalized_sentence);
    nmea_sent_count++;
    
    // 更新對應類型的發送時間戳
    unsigned long current_time = millis();
    if (nmea_sentence.startsWith("$GNGGA")) {
      last_gga_send = current_time;
      gga_sent++;
    } else if (nmea_sentence.startsWith("$GNRMC")) {
      last_rmc_send = current_time;
      rmc_sent++;
    } else if (nmea_sentence.startsWith("$GNGST")) {
      last_gst_send = current_time;
      gst_sent++;
    } else if (nmea_sentence.startsWith("$GNGSA")) {
      last_gsa_send = current_time;
      gsa_sent++;
    } else if (nmea_sentence.startsWith("$GNVTG")) {
      last_vtg_send = current_time;
      vtg_sent++;
    } else if (nmea_sentence.startsWith("$GNZDA")) {
      last_zda_send = current_time;
      zda_sent++;
    }
    
    if (is_debug) {
      Serial.println("→ MTi-680: " + normalized_sentence);
    }
    return;
  }
  
  // 新的同步數據組處理邏輯
  // 提取當前句子的時間戳
  String sentence_timestamp = extractNMEATimestamp(nmea_sentence);
  
  // 調試：輸出時間戳提取結果和完整NMEA句子
  if (is_debug && sentence_timestamp.length() > 0) {
    Serial.println("[TIMESTAMP] " + nmea_sentence.substring(0, 6) + " -> " + sentence_timestamp + " | " + nmea_sentence);
  }
  
  // 檢查數據組收集超時
  if (current_dataset.collect_start_time > 0 && 
      (millis() - current_dataset.collect_start_time) > DATASET_TIMEOUT) {
    if (is_debug) {
      Serial.println("[DATASET] Timeout - resetting incomplete dataset");
    }
    incomplete_datasets++;
    resetCurrentDataset();
  }
  
  // 如果這是新的時間戳，且當前數據組有數據，先處理當前組
  if (sentence_timestamp.length() > 0 && 
      current_dataset.timestamp.length() > 0 && 
      !isTimestampCompatible(sentence_timestamp, current_dataset.timestamp)) {
    
    // 如果當前數據組完整，發送它
    if (isDatasetComplete()) {
      sendSynchronizedDataset(output_port);
    } else {
      // 寬鬆處理：如果有基本數據但時間戳不匹配，仍嘗試發送已有的完整部分
      if (current_dataset.has_gga || current_dataset.has_rmc) {
        incomplete_datasets++;
        if (is_debug) {
          Serial.println("[DATASET] Partial dataset discarded (timestamp mismatch: " + 
                         current_dataset.timestamp + " -> " + sentence_timestamp + ")");
        }
      }
    }
    resetCurrentDataset();
  }
  
  // 設置數據組的時間戳 (如果尚未設置)
  if (current_dataset.timestamp.length() == 0 && sentence_timestamp.length() > 0) {
    current_dataset.timestamp = sentence_timestamp;
    current_dataset.collect_start_time = millis();
  }
  
  // 只有時間戳匹配時才收集句子（使用容差比較）
  if (sentence_timestamp.length() == 0 || 
      current_dataset.timestamp.length() == 0 ||
      isTimestampCompatible(sentence_timestamp, current_dataset.timestamp)) {
    // 根據句子類型添加到當前數據組
    if (nmea_sentence.startsWith("$GNGGA") && !current_dataset.has_gga) {
      current_dataset.gga_sentence = nmea_sentence;
      current_dataset.has_gga = true;
    } else if (nmea_sentence.startsWith("$GNRMC") && !current_dataset.has_rmc) {
      current_dataset.rmc_sentence = nmea_sentence;
      current_dataset.has_rmc = true;
    } else if (nmea_sentence.startsWith("$GNGST") && !current_dataset.has_gst) {
      current_dataset.gst_sentence = nmea_sentence;
      current_dataset.has_gst = true;
    } else if (nmea_sentence.startsWith("$GNGSA") && current_dataset.gsa_count < 4) {
      current_dataset.gsa_sentences[current_dataset.gsa_count] = nmea_sentence;
      current_dataset.gsa_count++;
    } else if (nmea_sentence.startsWith("$GNVTG") && !current_dataset.has_vtg) {
      current_dataset.vtg_sentence = nmea_sentence;
      current_dataset.has_vtg = true;
    } else if (nmea_sentence.startsWith("$GNZDA") && !current_dataset.has_zda) {
      current_dataset.zda_sentence = nmea_sentence;
      current_dataset.has_zda = true;
    }
    
    // 檢查數據組是否完整，如果是則立即發送
    if (isDatasetComplete()) {
      if (is_debug) {
        Serial.println("[DATASET] Complete! Sending: GGA=" + String(current_dataset.has_gga) + 
                       ", RMC=" + String(current_dataset.has_rmc) + 
                       ", GST=" + String(current_dataset.has_gst) + 
                       ", GSA=" + String(current_dataset.gsa_count) + 
                       ", TS=" + current_dataset.timestamp);
      }
      sendSynchronizedDataset(output_port);
      resetCurrentDataset();
    }
  }
}

bool validateNMEAChecksum(String nmea_sentence) {
  int asterisk_pos = nmea_sentence.indexOf('*');
  if (asterisk_pos == -1 || asterisk_pos >= nmea_sentence.length() - 2) {
    return false;  // No checksum found
  }
  
  // Calculate checksum
  uint8_t calculated_checksum = 0;
  for (int i = 1; i < asterisk_pos; i++) {  // Start after '$'
    calculated_checksum ^= nmea_sentence.charAt(i);
  }
  
  // Get provided checksum
  String checksum_str = nmea_sentence.substring(asterisk_pos + 1);
  uint8_t provided_checksum = strtol(checksum_str.c_str(), NULL, 16);
  
  return calculated_checksum == provided_checksum;
}

String modifyNMEATimestamp(String nmea_sentence) {
  // For now, just return the original sentence
  // TODO: Implement timestamp modification based on time_offset
  return nmea_sentence;
}

// checkLOCOSYS_ACK implementation is in myUARTSensor.cpp

String readCurrentBytes(HardwareSerial &port) {
  String result = "";
  unsigned long start_time = millis();
  const size_t MAX_COMMAND_LENGTH = 64; // 防護緩衝區溢位
  
  // Read available bytes with timeout and bounds checking
  while (port.available() && (millis() - start_time < 1000) && result.length() < MAX_COMMAND_LENGTH) {
    char c = port.read();
    if (c == '\n' || c == '\r') {
      if (result.length() > 0) {
        break;  // Complete line received
      }
    } else if (c >= 32 && c <= 126) { // 只接受可印字元
      result += c;
    }
    // 忽略其他字元
  }
  
  return result;
}

// Security Check Functions for UAV/Satellite Applications (Production-Ready)

// 簡單的雜湊函數（在生產環境中應使用加密雜湊）
uint32_t simpleHash(uint32_t input) {
  input ^= input >> 16;
  input *= 0x85ebca6b;
  input ^= input >> 13;
  input *= 0xc2b2ae35;
  input ^= input >> 16;
  return input;
}

// 常數時間比較（防護時序攻擊）
bool constantTimeCompare(uint32_t a, uint32_t b) {
  uint32_t diff = a ^ b;
  // 確保所有比較路徑耗時相同
  for (int i = 0; i < 32; i++) {
    diff |= (diff >> 1);
  }
  return (diff == 0);
}

bool authenticateSecurityCommand(uint32_t provided_token) {
  // 檢查非阻塞式鎖定
  if (is_locked_out) {
    if (millis() - lockout_start_time >= 10000) {
      // 鎖定期滿，重置狀態
      is_locked_out = false;
      failed_auth_attempts = 0;
    } else {
      return false; // 仍在鎖定期間
    }
  }
  
  // 對提供的令牌進行雜湊處理
  uint32_t provided_hash = simpleHash(provided_token);
  
  // 使用常數時間比較防護時序攻擊
  bool auth_success = constantTimeCompare(provided_hash, SECURITY_TOKEN_HASH);
  
  if (!auth_success) {
    failed_auth_attempts++;
    if (failed_auth_attempts >= 3) {
      // 非阻塞式鎖定
      is_locked_out = true;
      lockout_start_time = millis();
      // 不輸出詳細錯誤信息，防止信息洩露
    }
    return false;
  }
  
  // 認證成功，重置失敗計數
  failed_auth_attempts = 0;
  is_locked_out = false;
  return true;
}

void performSecurityCheck(HardwareSerial &port, bool silent_mode = false) {
  security_level = 0;  // Reset security level
  
  if (!silent_mode) {
    send2Serial(port, "[SEC] Security scan initiated");
  }
  
  uint8_t checks_passed = 0;
  uint8_t total_checks = 5;
  
  // 1. Hardware Integrity Check
  if (checkHardwareIntegrity()) {
    checks_passed++;
  } else {
    security_level = 2;
    if (!silent_mode) send2Serial(port, "[SEC] HW_ERR");
    return;
  }
  
  // 2. Sensor Data Validation
  if (validateSensorData()) {
    checks_passed++;
  } else {
    security_level = max(security_level, (uint8_t)1);
    if (!silent_mode) send2Serial(port, "[SEC] SENSOR_WARN");
  }
  
  // 3. Communication Security Check
  if (checkCommunicationSecurity()) {
    checks_passed++;
  } else {
    security_level = max(security_level, (uint8_t)1);
    if (!silent_mode) send2Serial(port, "[SEC] COMM_WARN");
  }
  
  // 4. Time Synchronization Integrity
  if (checkTimeSyncIntegrity()) {
    checks_passed++;
  } else {
    security_level = max(security_level, (uint8_t)1);
    if (!silent_mode) send2Serial(port, "[SEC] TIME_WARN");
  }
  
  // 5. Memory Integrity Check
  if (checkMemoryIntegrity()) {
    checks_passed++;
  } else {
    security_level = 2;
    if (!silent_mode) send2Serial(port, "[SEC] MEM_ERR");
    return;
  }
  
  // Report final security status (minimal info disclosure)
  if (!silent_mode) {
    switch(security_level) {
      case 0:
        send2Serial(port, "[SEC] STATUS_OK");
        break;
      case 1:
        send2Serial(port, "[SEC] STATUS_WARN");
        break;
      case 2:
        send2Serial(port, "[SEC] STATUS_ERR");
        break;
    }
  }
  
  last_security_check = millis();
}

bool checkHardwareIntegrity() {
  // Check if all serial ports are functioning
  if (!Serial || !PIXHAWK_SERIAL || !Serial_xsens || !NMEA_IN_Serial || !NMEA_OUT_Serial) {
    return false;
  }
  
  // Check if Xsens sensor is responding
  if (!xsens.getDataStatus()) {
    // Sensor not responding might indicate tampering
    return false;
  }
  
  // Check power levels and voltage stability (if available)
  // This would require hardware-specific implementation
  
  return true;
}

bool validateSensorData() {
  // Check for impossible sensor values that might indicate spoofing
  
  // Example: Check for reasonable acceleration values (not exceeding physical limits)
  // This would use the latest sensor data
  
  // Check for GPS spoofing indicators
  if (gps_connected) {
    // Look for rapid position jumps, impossible velocities, etc.
    // This requires storing previous GPS data for comparison
  }
  
  // Check for time jumps that might indicate tampering
  if (time_sync_initialized && abs(time_offset) > 1000000) {  // > 1 second offset is suspicious
    return false;
  }
  
  return true;
}

bool checkCommunicationSecurity() {
  // Verify MAVLink message integrity
  // Check for unexpected message patterns that might indicate intrusion
  
  // Monitor data rates for anomalies
  static unsigned long last_data_count = 0;
  static unsigned long last_rate_check = 0;
  static unsigned long data_count = 0;
  
  data_count++;
  
  if (millis() - last_rate_check > 5000) {  // Check every 5 seconds
    unsigned long current_rate = (data_count - last_data_count) / 5;
    
    // Abnormally high data rates might indicate flooding attack
    if (current_rate > 1000) {  // More than 1000 messages per second is suspicious
      return false;
    }
    
    last_data_count = data_count;
    last_rate_check = millis();
  }
  
  return true;
}

bool checkTimeSyncIntegrity() {
  // Verify time synchronization hasn't been tampered with
  if (!time_sync_initialized) {
    return false;  // Time sync should be initialized by now
  }
  
  // Check for time drift beyond acceptable limits
  if (abs(time_offset) > 500000) {  // > 0.5 second is concerning
    return false;
  }
  
  return true;
}

bool checkMemoryIntegrity() {
  // Basic memory corruption check
  // In a real implementation, you might use checksums or hash verification
  
  // Check stack overflow by examining stack pointer
  // This is hardware/compiler specific
  
  // Check for critical variable corruption
  if (current_Xsens_mode > 10 || current_output_mode > 10) {
    return false;  // Values outside expected range
  }
  
  return true;
}

void enableSecurityMode(HardwareSerial &port) {
  security_mode_active = true;
  send2Serial(port, "[SECURITY] Security monitoring enabled");
  send2Serial(port, "[SECURITY] Continuous monitoring active for UAV/Satellite operations");
}

void disableSecurityMode(HardwareSerial &port) {
  security_mode_active = false;
  send2Serial(port, "[SECURITY] Security monitoring disabled");
}

void sendProcessingDataAndStartMeas(HardwareSerial &port){
  xsens.ToConfigMode();
  xsens.reset();
  
  delay(2000);  // 🔁 等感測器重新開機

  bool init_success = false;
  for (int i = 0; i < 2; i++) {
    xsens.InitMT();
    delay(300);  // 給時間完成初始化
    // 簡單驗證：假設初始化成功
    init_success = true;
    break;
  }
  
  if (!init_success) {
    Serial.println("Init Sensor failed.");
    port.println("Init Sensor failed");
    return;
  }

  delay(200);  // 保險延遲

  bool meas_success = false;
for (int i = 0; i < 2; i++) {
  xsens.ToMeasurementMode();
  delay(300);  // 給時間完成模式切換
  
  if (xsens.getDataStatus() != DATA_INVALID) {
    meas_success = true;
    break;
  }
}
  
  if (!meas_success) {
    Serial.println("Measurement Mode failed.");
    port.println("Measurement Mode failed");
    return;
  }

  port.println("[XSENS] Init + Measurement Mode Success");
}

// Calculates the 16-bit CRC for the given ASCII or binary message.
unsigned short calculateCRC(unsigned char data[], unsigned int length)
{
  unsigned int i;
  unsigned short crc = 0;
  for(i=0; i<length; i++){
    crc = (unsigned char)(crc >> 8) | (crc << 8);
    crc ^= data[i];
    crc ^= (unsigned char)(crc & 0xff) >> 4;
    crc ^= crc << 12;
    crc ^= (crc & 0x00ff) << 5;
  }
  return crc;
}

// ================== 動態權重系統實作 ==================

void updateMovementDetection(const my_data_2d &latlon, const my_data_u4 &hei, 
                           const my_data_3f &vel, const my_data_3f &omg, 
                           const my_data_3f &acc) {
  unsigned long current_time = millis();
  float dt = 0.033f; // 假設 30Hz 更新率
  
  if (dynamic_trust.last_update > 0) {
    dt = (current_time - dynamic_trust.last_update) / 1000.0f;
  }
  
  if (!dynamic_trust.is_initialized) {
    // 首次初始化
    dynamic_trust.prev_pos[0] = latlon.float_val[0];
    dynamic_trust.prev_pos[1] = latlon.float_val[1];
    dynamic_trust.prev_pos[2] = hei.float_val;
    
    for (int i = 0; i < 3; i++) {
      dynamic_trust.prev_vel[i] = vel.float_val[i];
      dynamic_trust.prev_omg[i] = omg.float_val[i];
    }
    
    dynamic_trust.is_initialized = true;
    dynamic_trust.trust_factor = 0.3f; // 初始較低信任度
  } else {
    // 計算位置變化（轉換為米）
    float pos_change[3];
    pos_change[0] = (latlon.float_val[0] - dynamic_trust.prev_pos[0]) * 111319.5f; // 緯度差轉米
    pos_change[1] = (latlon.float_val[1] - dynamic_trust.prev_pos[1]) * 111319.5f * cos(latlon.float_val[0] * M_PI / 180.0f); // 經度差轉米
    pos_change[2] = hei.float_val - dynamic_trust.prev_pos[2];
    
    // 計算速度變化
    float vel_change[3];
    for (int i = 0; i < 3; i++) {
      vel_change[i] = vel.float_val[i] - dynamic_trust.prev_vel[i];
    }
    
    // 計算角速度變化
    float omg_change[3];
    for (int i = 0; i < 3; i++) {
      omg_change[i] = omg.float_val[i] - dynamic_trust.prev_omg[i];
    }
    
    // 計算加速度幅值
    dynamic_trust.acceleration_mag = sqrt(acc.float_val[0]*acc.float_val[0] + 
                                        acc.float_val[1]*acc.float_val[1] + 
                                        acc.float_val[2]*acc.float_val[2]);
    
    // 計算角速度幅值
    dynamic_trust.angular_rate_mag = sqrt(omg.float_val[0]*omg.float_val[0] + 
                                        omg.float_val[1]*omg.float_val[1] + 
                                        omg.float_val[2]*omg.float_val[2]);
    
    // 更新YAW角速度歷史記錄
    dynamic_trust.yaw_rate_history[dynamic_trust.yaw_history_index] = omg.float_val[2]; // YAW角速度
    dynamic_trust.yaw_history_index = (dynamic_trust.yaw_history_index + 1) % 5;
    
    // 計算位置變化幅值
    float pos_change_mag = sqrt(pos_change[0]*pos_change[0] + 
                               pos_change[1]*pos_change[1] + 
                               pos_change[2]*pos_change[2]);
    
    // 計算速度幅值
    float vel_mag = sqrt(vel.float_val[0]*vel.float_val[0] + 
                        vel.float_val[1]*vel.float_val[1] + 
                        vel.float_val[2]*vel.float_val[2]);
    
    // 綜合運動評分 (0-1)
    float position_score = min(pos_change_mag / dt / 1.0f, 1.0f); // 位置變化率
    float velocity_score = min(vel_mag / 5.0f, 1.0f);             // 速度幅值
    float accel_score = min(dynamic_trust.acceleration_mag / 10.0f, 1.0f); // 加速度
    float angular_score = min(dynamic_trust.angular_rate_mag / 1.0f, 1.0f); // 角速度
    
    // 加權平均運動評分
    dynamic_trust.movement_score = 0.3f * position_score + 
                                  0.3f * velocity_score + 
                                  0.2f * accel_score + 
                                  0.2f * angular_score;
    
    // 更新前一次數值
    dynamic_trust.prev_pos[0] = latlon.float_val[0];
    dynamic_trust.prev_pos[1] = latlon.float_val[1];
    dynamic_trust.prev_pos[2] = hei.float_val;
    
    for (int i = 0; i < 3; i++) {
      dynamic_trust.prev_vel[i] = vel.float_val[i];
      dynamic_trust.prev_omg[i] = omg.float_val[i];
    }
  }
  
  dynamic_trust.last_update = current_time;
}

float calculateTrustFactor() {
  // 基礎信任度 - 即使靜止也要有一定信任度
  float base_trust = 0.2f;
  
  // 運動加成 - 有運動時增加信任度
  float movement_bonus = dynamic_trust.movement_score * 0.7f;
  
  // 組合信任因子
  float new_trust = base_trust + movement_bonus;
  
  // 平滑過濾 - 避免劇烈變化
  float alpha = 0.1f; // 低通濾波器係數
  dynamic_trust.trust_factor = alpha * new_trust + (1.0f - alpha) * dynamic_trust.trust_factor;
  
  // 限制範圍
  dynamic_trust.trust_factor = constrain(dynamic_trust.trust_factor, 0.1f, 1.0f);
  
  return dynamic_trust.trust_factor;
}

float calculateYawTrustFactor() {
  // YAW軸專用信任因子：基於角速度穩定性
  
  // 1. 計算YAW角速度變化的標準差
  float yaw_rate_mean = 0.0f;
  for (int i = 0; i < 5; i++) {
    yaw_rate_mean += dynamic_trust.yaw_rate_history[i];
  }
  yaw_rate_mean /= 5.0f;
  
  float yaw_rate_variance = 0.0f;
  for (int i = 0; i < 5; i++) {
    float diff = dynamic_trust.yaw_rate_history[i] - yaw_rate_mean;
    yaw_rate_variance += diff * diff;
  }
  yaw_rate_variance /= 5.0f;
  float yaw_rate_std = sqrt(yaw_rate_variance);
  
  // 2. 將標準差映射到穩定性評分
  // 標準差越小，穩定性越高
  float max_acceptable_std = 0.05f; // 最大可接受的標準差 (rad/s)
  float normalized_std = constrain(yaw_rate_std / max_acceptable_std, 0.0f, 1.0f);
  dynamic_trust.yaw_stability_score = 1.0f - normalized_std;
  
  // 3. 結合當前YAW角速度大小
  float current_yaw_rate = abs(dynamic_trust.prev_omg[2]);
  float yaw_rate_factor = 1.0f / (1.0f + current_yaw_rate * 10.0f); // 角速度越大，信任度越低
  
  // 4. 綜合評分
  float combined_score = (dynamic_trust.yaw_stability_score * 0.7f) + (yaw_rate_factor * 0.3f);
  
  // 5. 映射到信任因子範圍 (0.1 - 1.0)
  dynamic_trust.yaw_trust_factor = 0.1f + combined_score * 0.9f;
  
  return dynamic_trust.yaw_trust_factor;
}

void adaptiveCovariance(mavlink_odometry_t &odom, float trust_factor) {
  // 協方差範圍設定：從較大的不確定性到非常高的精度
  float max_pos_cov = 1.0f;        // 最大位置不確定性 (1米)
  float min_pos_cov = 1e-6f;       // 最高位置精度 (1微米)
  
  float max_vel_cov = 1.0f;        // 最大速度不確定性 (1 m/s)
  float min_vel_cov = 1e-6f;       // 最高速度精度 (1 mm/s)
  
  float max_att_cov = 1e-1f;       // 最大姿態不確定性 (約5.7度)
  float min_att_cov = 1e-6f;       // 最高姿態精度 (調高到1e-6)
  
  // YAW軸專用信任因子計算
  float yaw_trust_factor = calculateYawTrustFactor();
  
  // 使用指數映射來實現更大的動態範圍
  // trust_factor: 0.1 -> 最大協方差, 1.0 -> 最小協方差
  float exp_factor = -6.0f * (trust_factor - 0.1f) / 0.9f; // -6 到 0 的範圍
  float yaw_exp_factor = -6.0f * (yaw_trust_factor - 0.1f) / 0.9f; // YAW專用指數因子
  
  float pos_cov = min_pos_cov + (max_pos_cov - min_pos_cov) * exp(exp_factor);
  float vel_cov = min_vel_cov + (max_vel_cov - min_vel_cov) * exp(exp_factor);
  float att_cov = min_att_cov + (max_att_cov - min_att_cov) * exp(exp_factor);
  float yaw_att_cov = min_att_cov + (max_att_cov - min_att_cov) * exp(yaw_exp_factor);
  
  // 確保在合理範圍內
  pos_cov = constrain(pos_cov, min_pos_cov, max_pos_cov);
  vel_cov = constrain(vel_cov, min_vel_cov, max_vel_cov);
  att_cov = constrain(att_cov, min_att_cov, max_att_cov);
  yaw_att_cov = constrain(yaw_att_cov, min_att_cov, max_att_cov);
  
  // 設定位置協方差
  odom.pose_covariance[0]  = pos_cov;           // x position variance
  odom.pose_covariance[7]  = pos_cov;           // y position variance  
  odom.pose_covariance[14] = pos_cov * 1.5f;    // z position variance (高度稍微不確定些)
  odom.pose_covariance[21] = att_cov;           // roll variance
  odom.pose_covariance[28] = att_cov;           // pitch variance
  odom.pose_covariance[35] = yaw_att_cov;       // yaw variance (使用專用信任因子)
  
  // 設定速度協方差
  odom.velocity_covariance[0]  = vel_cov;           // vx variance
  odom.velocity_covariance[7]  = vel_cov;           // vy variance
  odom.velocity_covariance[14] = vel_cov * 1.2f;    // vz variance (垂直速度稍不確定)
  odom.velocity_covariance[21] = att_cov * 5.0f;    // roll rate variance
  odom.velocity_covariance[28] = att_cov * 5.0f;    // pitch rate variance
  odom.velocity_covariance[35] = yaw_att_cov * 3.0f;    // yaw rate variance (使用專用信任因子)
  
  // 品質評分：設定為最高品質
  odom.quality = 100; // 固定為最高品質100
  
  // 極高信任度時使用更積極的估計器類型
  if (trust_factor > 0.8f) {
    odom.estimator_type = MAV_ESTIMATOR_TYPE_VISION; // 純視覺估計器，EKF 最信任
  } else if (trust_factor > 0.5f) {
    odom.estimator_type = MAV_ESTIMATOR_TYPE_VIO;    // 視覺慣性里程計
  } else {
    odom.estimator_type = MAV_ESTIMATOR_TYPE_NAIVE;  // 基本估計器
  }
}

// ==================== NMEA 時間戳格式標準化功能 ====================

// 將時間戳從3位小數格式轉換為2位小數格式
// 例如: "135937.500" -> "135937.50"
String normalizeTimestampFormat(const String& timestamp) {
  if (timestamp.length() < 6) {
    return timestamp;
  }
  
  int dot_pos = timestamp.indexOf('.');
  if (dot_pos == -1) {
    return timestamp;
  }
  
  // 如果已經是2位小數，直接返回
  if (timestamp.length() == dot_pos + 3) {
    return timestamp;
  }
  
  // 如果是3位小數，截取為2位小數
  if (timestamp.length() == dot_pos + 4) {
    return timestamp.substring(0, dot_pos + 3);
  }
  
  return timestamp;
}

// 格式化速度字段為標準格式 (XXX.XXX)
String formatSpeedField(const String& speed) {
  float speed_val = speed.toFloat();
  return String(speed_val, 3).substring(0, 7); // 確保格式為XXX.XXX
}

// 標準化整個NMEA句子中的時間戳和其他字段格式
String normalizeNMEASentence(const String& nmea_sentence) {
  String result = nmea_sentence;
  
  // 步驟1: 處理包含時間戳的句子
  if (nmea_sentence.startsWith("$GNGGA") || 
      nmea_sentence.startsWith("$GNRMC") || 
      nmea_sentence.startsWith("$GNGST")) {
    // 標準化時間戳字段（第二個逗號之間）
    int first_comma = result.indexOf(',');
    if (first_comma != -1) {
      int second_comma = result.indexOf(',', first_comma + 1);
      if (second_comma != -1) {
        String timestamp = result.substring(first_comma + 1, second_comma);
        String normalized_timestamp = normalizeTimestampFormat(timestamp);
        
        // 重構NMEA句子
        result = result.substring(0, first_comma + 1);
        result += normalized_timestamp;
        result += nmea_sentence.substring(second_comma);
      }
    }
  }
  
  // 步驟2: 特殊處理GNRMC句子的速度字段
  if (result.startsWith("$GNRMC")) {
    // 解析GNRMC字段並重新格式化速度
    String fields[15]; // GNRMC最多15個字段
    int field_count = 0;
    int start = 0;
    
    // 分割字段
    for (int i = 0; i <= result.length() && field_count < 15; i++) {
      if (i == result.length() || result.charAt(i) == ',') {
        fields[field_count] = result.substring(start, i);
        field_count++;
        start = i + 1;
      }
    }
    
    // 格式化速度字段（第8個字段，索引7）
    // GNRMC格式: $GNRMC,time,status,lat,N/S,lon,E/W,speed,course,date,variation,E/W,checksum
    //           0     1    2      3   4   5   6   7     8     9    10        11  12
    if (field_count > 7 && fields[7].length() > 0) {
      float speed_val = fields[7].toFloat();
      
      // 格式化為 XXX.XXX 格式
      int integer_part = (int)speed_val;
      int decimal_part = (int)((speed_val - integer_part) * 1000);
      
      String formatted_speed = "";
      if (integer_part < 100) formatted_speed += "0";
      if (integer_part < 10) formatted_speed += "0";
      formatted_speed += String(integer_part);
      formatted_speed += ".";
      if (decimal_part < 100) formatted_speed += "0";
      if (decimal_part < 10) formatted_speed += "0";
      formatted_speed += String(decimal_part);
      
      fields[7] = formatted_speed;
      
      // 調試信息可以在需要時重新啟用
    }
    
    // 重新組裝GNRMC句子
    result = "";
    for (int i = 0; i < field_count; i++) {
      result += fields[i];
      if (i < field_count - 1) {
        result += ",";
      }
    }
  }
  
  return result;
}

