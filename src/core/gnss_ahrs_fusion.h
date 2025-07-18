#ifndef GNSS_AHRS_FUSION_H
#define GNSS_AHRS_FUSION_H

#include <Arduino.h>
#include <MAVLink.h>
#include "../communication/myMessage.h"

// 融合模組狀態
struct FusionStatus {
    bool gnss_valid = false;
    bool mtdata2_valid = false;
    bool fusion_active = false;
    bool mavlink_sent = false;
    unsigned long last_gnss_update = 0;
    unsigned long last_mtdata2_update = 0;
    unsigned long last_fusion_send = 0;
    int gnss_quality = 0;
    int fusion_count = 0;
    int mavlink_send_count = 0;
};

// GNSS 資料結構
struct GNSSData {
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double velocity_north = 0.0;
    double velocity_east = 0.0;
    double velocity_down = 0.0;
    double course = 0.0;
    double speed = 0.0;
    int quality = 0;
    bool valid = false;
    unsigned long timestamp = 0;
};

// MTDATA2 資料結構
struct MTData2 {
    float quaternion[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    float angular_velocity[3] = {0.0f, 0.0f, 0.0f};
    float acceleration[3] = {0.0f, 0.0f, 0.0f};
    float orientation[3] = {0.0f, 0.0f, 0.0f};
    uint32_t timestamp = 0;
    bool valid = false;
};

// 融合模組類別
class GNSSAHRSFusion {
private:
    GNSSData gnss_data;
    MTData2 mtdata2_data;
    FusionStatus status;
    bool debug_enabled = false;
    
    // 內部函數
    void debugPrint(const String& message);
    void updateFusionStatus();
    bool isDataReady();
    
public:
    GNSSAHRSFusion();
    
    // 設定函數
    void setDebugMode(bool enabled);
    
    // 資料輸入函數
    void updateGNSSData(const String& nmea_sentence);
    void updateMTData2(const my_data_4f& quat, const my_data_3f& omg, 
                      const my_data_3f& acc, const my_data_3f& ori, uint32_t timestamp);
    
    // 主要融合函數
    bool processFusion();
    
    // MAVLink 輸出函數
    bool sendMAVLinkOdometry(HardwareSerial& serial_port);
    
    // 狀態查詢函數
    FusionStatus getFusionStatus();
    void printDetailedStatus();
    
    // 測試函數
    void runDiagnostics();
};

#endif // GNSS_AHRS_FUSION_H