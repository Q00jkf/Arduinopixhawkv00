#include "gnss_ahrs_fusion.h"

// 建構函數
GNSSAHRSFusion::GNSSAHRSFusion() {
    status.gnss_valid = false;
    status.mtdata2_valid = false;
    status.fusion_active = false;
    status.mavlink_sent = false;
    status.last_gnss_update = 0;
    status.last_mtdata2_update = 0;
    status.last_fusion_send = 0;
    status.gnss_quality = 0;
    status.fusion_count = 0;
    status.mavlink_send_count = 0;
}

// 設定除錯模式
void GNSSAHRSFusion::setDebugMode(bool enabled) {
    debug_enabled = enabled;
    if (debug_enabled) {
        debugPrint("[FUSION] Debug mode enabled");
    }
}

// 除錯輸出函數
void GNSSAHRSFusion::debugPrint(const String& message) {
    if (debug_enabled) {
        Serial.println(message);
    }
}

// 更新 GNSS 資料
void GNSSAHRSFusion::updateGNSSData(const String& nmea_sentence) {
    if (nmea_sentence.length() < 10) return;
    
    // 解析 GGA 句子
    if (nmea_sentence.startsWith("$GNGGA") || nmea_sentence.startsWith("$GPGGA")) {
        debugPrint("[FUSION] Processing GGA: " + nmea_sentence.substring(0, 20) + "...");
        
        // 簡單的 NMEA 解析
        int comma_positions[15];
        int comma_count = 0;
        
        // 找出所有逗號位置
        for (int i = 0; i < nmea_sentence.length() && comma_count < 15; i++) {
            if (nmea_sentence[i] == ',') {
                comma_positions[comma_count] = i;
                comma_count++;
            }
        }
        
        if (comma_count >= 10) {
            // 提取緯度 (欄位 2,3)
            String lat_str = nmea_sentence.substring(comma_positions[1] + 1, comma_positions[2]);
            String lat_dir = nmea_sentence.substring(comma_positions[2] + 1, comma_positions[3]);
            
            // 提取經度 (欄位 4,5)
            String lon_str = nmea_sentence.substring(comma_positions[3] + 1, comma_positions[4]);
            String lon_dir = nmea_sentence.substring(comma_positions[4] + 1, comma_positions[5]);
            
            // 提取品質 (欄位 6)
            String quality_str = nmea_sentence.substring(comma_positions[5] + 1, comma_positions[6]);
            
            // 提取高度 (欄位 9)
            String alt_str = nmea_sentence.substring(comma_positions[8] + 1, comma_positions[9]);
            
            if (lat_str.length() > 4 && lon_str.length() > 5) {
                // 轉換緯度 (ddmm.mmmm)
                double lat_deg = lat_str.substring(0, 2).toDouble();
                double lat_min = lat_str.substring(2).toDouble();
                gnss_data.latitude = lat_deg + lat_min / 60.0;
                if (lat_dir == "S") gnss_data.latitude = -gnss_data.latitude;
                
                // 轉換經度 (dddmm.mmmm)
                double lon_deg = lon_str.substring(0, 3).toDouble();
                double lon_min = lon_str.substring(3).toDouble();
                gnss_data.longitude = lon_deg + lon_min / 60.0;
                if (lon_dir == "W") gnss_data.longitude = -gnss_data.longitude;
                
                // 高度和品質
                gnss_data.altitude = alt_str.toDouble();
                gnss_data.quality = quality_str.toInt();
                
                gnss_data.valid = (gnss_data.quality > 0);
                gnss_data.timestamp = millis();
                status.last_gnss_update = millis();
                
                debugPrint("[FUSION] GNSS Updated: Lat=" + String(gnss_data.latitude, 6) + 
                          " Lon=" + String(gnss_data.longitude, 6) + 
                          " Alt=" + String(gnss_data.altitude, 1) + 
                          " Q=" + String(gnss_data.quality));
            }
        }
    }
    
    // 解析 RMC 句子
    else if (nmea_sentence.startsWith("$GNRMC") || nmea_sentence.startsWith("$GPRMC")) {
        debugPrint("[FUSION] Processing RMC: " + nmea_sentence.substring(0, 20) + "...");
        
        int comma_positions[12];
        int comma_count = 0;
        
        for (int i = 0; i < nmea_sentence.length() && comma_count < 12; i++) {
            if (nmea_sentence[i] == ',') {
                comma_positions[comma_count] = i;
                comma_count++;
            }
        }
        
        if (comma_count >= 8) {
            // 提取速度 (欄位 7) 和航向 (欄位 8)
            String speed_str = nmea_sentence.substring(comma_positions[6] + 1, comma_positions[7]);
            String course_str = nmea_sentence.substring(comma_positions[7] + 1, comma_positions[8]);
            
            if (speed_str.length() > 0 && course_str.length() > 0) {
                gnss_data.speed = speed_str.toDouble() * 0.514444; // knots to m/s
                gnss_data.course = course_str.toDouble();
                
                // 計算速度分量
                double course_rad = gnss_data.course * M_PI / 180.0;
                gnss_data.velocity_north = gnss_data.speed * cos(course_rad);
                gnss_data.velocity_east = gnss_data.speed * sin(course_rad);
                gnss_data.velocity_down = 0.0;
                
                debugPrint("[FUSION] GNSS Velocity: Speed=" + String(gnss_data.speed, 3) + 
                          " Course=" + String(gnss_data.course, 1) + 
                          " VN=" + String(gnss_data.velocity_north, 3) + 
                          " VE=" + String(gnss_data.velocity_east, 3));
            }
        }
    }
    
    updateFusionStatus();
}

// 更新 MTDATA2 資料
void GNSSAHRSFusion::updateMTData2(const my_data_4f& quat, const my_data_3f& omg, 
                                   const my_data_3f& acc, const my_data_3f& ori, uint32_t timestamp) {
    // 複製四元數
    for (int i = 0; i < 4; i++) {
        mtdata2_data.quaternion[i] = quat.float_val[i];
    }
    
    // 複製角速度
    for (int i = 0; i < 3; i++) {
        mtdata2_data.angular_velocity[i] = omg.float_val[i];
        mtdata2_data.acceleration[i] = acc.float_val[i];
        mtdata2_data.orientation[i] = ori.float_val[i];
    }
    
    mtdata2_data.timestamp = timestamp;
    mtdata2_data.valid = true;
    status.last_mtdata2_update = millis();
    
    debugPrint("[FUSION] MTDATA2 Updated: Q0=" + String(mtdata2_data.quaternion[0], 3) + 
              " Q1=" + String(mtdata2_data.quaternion[1], 3) + 
              " Q2=" + String(mtdata2_data.quaternion[2], 3) + 
              " Q3=" + String(mtdata2_data.quaternion[3], 3));
    
    updateFusionStatus();
}

// 更新融合狀態
void GNSSAHRSFusion::updateFusionStatus() {
    unsigned long current_time = millis();
    
    // 檢查資料有效性 (2秒內的資料視為有效)
    status.gnss_valid = gnss_data.valid && (current_time - status.last_gnss_update < 2000);
    status.mtdata2_valid = mtdata2_data.valid && (current_time - status.last_mtdata2_update < 2000);
    
    // 融合狀態
    status.fusion_active = status.gnss_valid && status.mtdata2_valid;
    status.gnss_quality = gnss_data.quality;
    
    if (status.fusion_active) {
        debugPrint("[FUSION] Status: ACTIVE - GNSS:" + String(status.gnss_valid ? "OK" : "FAIL") + 
                  " MTDATA2:" + String(status.mtdata2_valid ? "OK" : "FAIL"));
    }
}

// 檢查資料是否準備好
bool GNSSAHRSFusion::isDataReady() {
    return status.fusion_active;
}

// 處理融合
bool GNSSAHRSFusion::processFusion() {
    if (!isDataReady()) {
        debugPrint("[FUSION] Data not ready for fusion");
        return false;
    }
    
    status.fusion_count++;
    debugPrint("[FUSION] Processing fusion #" + String(status.fusion_count));
    
    return true;
}

// 發送 MAVLink ODO 封包
bool GNSSAHRSFusion::sendMAVLinkOdometry(HardwareSerial& serial_port) {
    if (!processFusion()) {
        return false;
    }
    
    // 創建 MAVLink odometry 封包
    mavlink_odometry_t odom = {};
    
    // 時間戳
    odom.time_usec = (uint64_t)mtdata2_data.timestamp * 100UL;
    
    // 座標系設定
    odom.frame_id = MAV_FRAME_GLOBAL;
    odom.child_frame_id = MAV_FRAME_LOCAL_NED;
    
    // 位置 (從 GNSS)
    odom.x = gnss_data.latitude;
    odom.y = gnss_data.longitude;
    odom.z = -gnss_data.altitude; // NED 座標系
    
    // 速度 (從 GNSS)
    odom.vx = gnss_data.velocity_north;
    odom.vy = gnss_data.velocity_east;
    odom.vz = gnss_data.velocity_down;
    
    // 姿態 (從 MTDATA2)
    odom.q[0] = mtdata2_data.quaternion[0];
    odom.q[1] = mtdata2_data.quaternion[1];
    odom.q[2] = mtdata2_data.quaternion[2];
    odom.q[3] = mtdata2_data.quaternion[3];
    
    // 角速度 (從 MTDATA2)
    odom.rollspeed = mtdata2_data.angular_velocity[0];
    odom.pitchspeed = mtdata2_data.angular_velocity[1];
    odom.yawspeed = mtdata2_data.angular_velocity[2];
    
    // 協方差設定 (基於 GNSS 品質)
    float pos_variance = (gnss_data.quality == 2) ? 1.0f : 5.0f; // RTK vs 標準
    float vel_variance = 0.1f;
    float att_variance = 0.01f;
    
    // 位置協方差
    odom.pose_covariance[0] = pos_variance;  // x
    odom.pose_covariance[7] = pos_variance;  // y
    odom.pose_covariance[14] = pos_variance; // z
    
    // 速度協方差
    odom.velocity_covariance[0] = vel_variance;  // vx
    odom.velocity_covariance[7] = vel_variance;  // vy
    odom.velocity_covariance[14] = vel_variance; // vz
    
    // 設定估計器類型
    odom.estimator_type = MAV_ESTIMATOR_TYPE_GPS;
    odom.reset_counter = 0;
    
    // 發送 MAVLink 封包
    mavlink_message_t msg;
    mavlink_msg_odometry_encode(1, 200, &msg, &odom);
    
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    
    serial_port.write(buffer, len);
    
    // 更新狀態
    status.mavlink_sent = true;
    status.mavlink_send_count++;
    status.last_fusion_send = millis();
    
    debugPrint("[FUSION] MAVLink ODO sent #" + String(status.mavlink_send_count) + 
              " Length:" + String(len) + " bytes");
    debugPrint("[FUSION] Position: X=" + String(odom.x, 6) + 
              " Y=" + String(odom.y, 6) + 
              " Z=" + String(odom.z, 2));
    debugPrint("[FUSION] Velocity: VX=" + String(odom.vx, 3) + 
              " VY=" + String(odom.vy, 3) + 
              " VZ=" + String(odom.vz, 3));
    
    return true;
}

// 取得融合狀態
FusionStatus GNSSAHRSFusion::getFusionStatus() {
    updateFusionStatus();
    return status;
}

// 列印詳細狀態
void GNSSAHRSFusion::printDetailedStatus() {
    updateFusionStatus();
    unsigned long current_time = millis();
    
    debugPrint("=============== FUSION STATUS ===============");
    debugPrint("[FUSION] Active: " + String(status.fusion_active ? "YES" : "NO"));
    debugPrint("[FUSION] Count: " + String(status.fusion_count));
    debugPrint("[FUSION] MAVLink Sent: " + String(status.mavlink_send_count));
    
    debugPrint("[GNSS] Valid: " + String(status.gnss_valid ? "YES" : "NO"));
    debugPrint("[GNSS] Quality: " + String(status.gnss_quality));
    debugPrint("[GNSS] Age: " + String(current_time - status.last_gnss_update) + "ms");
    debugPrint("[GNSS] Lat: " + String(gnss_data.latitude, 6));
    debugPrint("[GNSS] Lon: " + String(gnss_data.longitude, 6));
    debugPrint("[GNSS] Alt: " + String(gnss_data.altitude, 1));
    debugPrint("[GNSS] Speed: " + String(gnss_data.speed, 3) + " m/s");
    
    debugPrint("[MTDATA2] Valid: " + String(status.mtdata2_valid ? "YES" : "NO"));
    debugPrint("[MTDATA2] Age: " + String(current_time - status.last_mtdata2_update) + "ms");
    debugPrint("[MTDATA2] Q0: " + String(mtdata2_data.quaternion[0], 3));
    debugPrint("[MTDATA2] Q1: " + String(mtdata2_data.quaternion[1], 3));
    debugPrint("[MTDATA2] Q2: " + String(mtdata2_data.quaternion[2], 3));
    debugPrint("[MTDATA2] Q3: " + String(mtdata2_data.quaternion[3], 3));
    
    debugPrint("=============================================");
}

// 診斷函數
void GNSSAHRSFusion::runDiagnostics() {
    debugPrint("[FUSION] Running diagnostics...");
    printDetailedStatus();
    
    if (!status.gnss_valid) {
        debugPrint("[FUSION] DIAGNOSTIC: GNSS data is invalid or too old");
    }
    
    if (!status.mtdata2_valid) {
        debugPrint("[FUSION] DIAGNOSTIC: MTDATA2 data is invalid or too old");
    }
    
    if (status.fusion_active) {
        debugPrint("[FUSION] DIAGNOSTIC: All systems operational");
    } else {
        debugPrint("[FUSION] DIAGNOSTIC: Fusion inactive - check data sources");
    }
}