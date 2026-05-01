/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */
#include "watering_application.h"
#include <cJSON.h>
#include <WiFi.h>
#include "src/framework/sys/log.h"
#include "src/framework/board/board.h"
#include "src/framework/board/wifi_board.h"
#include "src/framework/lang/lang_zh_cn.h"
#include "src/framework/display/u8g2_display.h"
#include "src/framework/sys/settings.h"
#include "src/framework/wifi/wifi_station.h"
#include "src/framework/peripheral/sensor.h"
#include "src/framework/peripheral/single_motor_driver.h"
#include "watering_config.h"
#include "esp32_mini_board.h"

#define TAG "WateringApplication"

// 传感器数据采集间隔
static const int kCollectInterval = 120000;  //ms

// 上传数据后等待多少秒（等待回调）
static const int kWaitSecondsAfterPublished = 30;  //second

// 每个周期采集多少次数据
static const int kTotalCountPerPeriod = 20;

// 深度睡眠时长（23小时）
static const long kDeepSleepSeconds = 23 * 3600;  //second

void* create_application() {
    return new WateringApplication();
}

WateringApplication::WateringApplication() : Application() { 

}

void WateringApplication::OnInit() {

    mqtt_service_ = new MqttService();

    Board& board = Board::GetInstance();
    
    WateringConfig& config = WateringConfig::GetInstance();
    
    // 启动传感器
    std::shared_ptr<Sensor> soil_mositure_ptr = board.GetSensor(kSoilMositureName);
    if (soil_mositure_ptr != nullptr) {
        soil_mositure_ptr->ReadData();
        soil_mositure_ptr->Start(kCollectInterval);  //120s 
    }

    started_ = true;
}

void WateringApplication::OnLoop() {

    ESP32_MINI_BOARD *board = static_cast<ESP32_MINI_BOARD*>(&Board::GetInstance());
    board->ButtonTick();

    delay(1);
}

void WateringApplication::ShowWifiConfigHit(const std::string& ssid, const std::string& config_url, const std::string& mac_address) {
    // 显示 WiFi 配置 AP 的 SSID 和 Web 服务器 URL
    Board& board = Board::GetInstance();
    U8g2Display* display = static_cast<U8g2Display*>(board.GetDisplay());
    display->SetStatus("配置网络");
    display->GetWindow()->SetText(1, "热点" + ssid);
    display->GetWindow()->SetText(2, "访问" + config_url);
}

/**
 * 物理按键事件处理
 */
bool WateringApplication::OnPhysicalButtonEvent(const std::string& button_name, const ButtonAction action) {

    Log::Info(TAG, "响应按钮：%s 的操作：%d", button_name.c_str(), action);

    if (button_name == kManualButton) {

        if (action == ButtonAction::DoubleClick) {
            DoWatering(5);
            return true;
        }

    }

    return Application::OnPhysicalButtonEvent(button_name, action);

}

/**
 * 传感器数据事件处理
 */
bool WateringApplication::OnSensorDataEvent(const std::string& sensor_name, const SensorValue& value) {

    if (sensor_name == kSoilMositureName) {
        Log::Info(TAG, "接收到传感器: %s 的数据: %d", sensor_name.c_str(), value.intValue());

        WifiBoard* wifi_board = static_cast<WifiBoard*>(&Board::GetInstance());
        U8g2Display* display = static_cast<U8g2Display*>(wifi_board->GetDisplay());

        if (value.intValue() == 0) {
            display->GetWindow()->SetText(1, "湿度: nodata");
            return false;
        }

        // 4095-1 映射到 1-100
        soil_moilture_value_ = map(value.intValue(), 1, 4095, 100, 1);

        if (!started_) {
            display->GetWindow()->SetText(1, "湿度: " + std::to_string(soil_moilture_value_) );
            return false;
        }

        display->GetWindow()->SetText(1, "湿度: " + std::to_string(soil_moilture_value_) );

        SetDeviceState(kDeviceStateConnecting);

        // 连接WiFi
        WiFi.mode(WIFI_STA);
#if CONFIG_WIFI_CONFIGURE_ENABLE==1
        // 使用配置信息连接
        if (!wifi_board->StartNetwork(30000)) {
            Log::Info(TAG, "WiFi连接失败。");
            SetDeviceState(kDeviceStateWarning);
            ShowMessage("WiFi连接失败。");
            return false;
        }
#else
        Log::Warn(TAG, "未配置WiFi连接信息。");
        SetDeviceState(kDeviceStateWarning);
        return false;
#endif

        Log::Info(TAG, "LocalIP: %s", wifi_board->GetIpAddress().c_str());

        WateringConfig& config = WateringConfig::GetInstance();
        if (config.mqtt_username()=="") {
            Log::Info(TAG, "mqtt username is blank。");
            ShowMessage("配置有误，请重新配置");
            
            // 关闭WiFi连接（省电）
            wifi_board->Disconnect();
            WiFi.mode(WIFI_OFF);
            return false;
        }

        int state = mqtt_service_->Connect(config.mqtt_server(), config.mqtt_username(), config.mqtt_password(), 5);
        // 连接MQTT
        if (state!=0) {
            // 连接失败
            Log::Info(TAG, "未连接到IoT平台。");
            SetDeviceState(kDeviceStateWarning);
            ShowMessage("IoT连接失败。");
            
            // 关闭WiFi连接（省电）
            wifi_board->Disconnect();
            WiFi.mode(WIFI_OFF);
            return false;
        }

        // 主题订阅
        mqtt_service_->SubscribeTopic(config.pump_control_topic(), [this, config](const std::string& payload) {
            OnIotMessageEvent(config.pump_control_topic(), payload);
        });

        SetDeviceState(kDeviceStateWorking);

        // 数据包处理
        // 创建JSON对象
        cJSON *temp_node = cJSON_CreateObject();
        cJSON_AddNumberToObject(temp_node, config.soil_moisture_dataname().c_str(), soil_moilture_value_);
        cJSON *root_node = cJSON_CreateObject();
        cJSON_AddItemToObject(root_node, "data", temp_node);

        // 发送JSON响应
        char *json_str = cJSON_PrintUnformatted(root_node);
        cJSON_Delete(root_node);

        // 发布数据到IoT
        Log::Info(TAG, "发布数据到IoT，主题: %s", config.soil_moisture_topic().c_str());
        mqtt_service_->PublishData(config.soil_moisture_topic(), std::string(json_str));

        // 等待30秒
        vTaskDelay(pdMS_TO_TICKS(kWaitSecondsAfterPublished * 1000));

        // 关闭IoT连接（省电）
        mqtt_service_->Disconnect();
        // 关闭WiFi连接（省电）
        wifi_board->Disconnect();
        WiFi.mode(WIFI_OFF);

        collect_count_++;
        // 已经采集了10次，进入深度睡眠23h
        if (collect_count_ > kTotalCountPerPeriod) {
            //    sleepSeconds<3600 ? sleepSeconds/60 : sleepSeconds/3600,
            //    sleepSeconds<3600 ? "分钟" : "小时");
            wifi_board->GetDisplay()->Sleep();
            wifi_board->Sleep(kDeepSleepSeconds * 1000);
        }

        return true;
    }

    return false;
}

/**
 * 物联网消息事件处理
 */
void WateringApplication::OnIotMessageEvent(const std::string& topic, const std::string& payload) {

    Log::Info(TAG, "处理IoT消息，主题：%s", topic.c_str());

    WateringConfig& config = WateringConfig::GetInstance();

    if (config.pump_control_topic() == topic) {
        // 浇水动作。
        cJSON *json = cJSON_Parse(payload.c_str());
        cJSON *data = cJSON_GetObjectItem(json, "data");
        if (data) {
            int state = cJSON_GetObjectItem(data, "state")->valueint;
            int keep_seconds = cJSON_GetObjectItem(data, "keepSeconds")->valueint;
            if (state==1) {
                DoWatering(keep_seconds);
            }
        } else {
            Log::Error(TAG, "Payload invalid.");
        }
    }

}

void WateringApplication::DoWatering(uint8_t seconds) {
    Schedule([seconds]() {
        Log::Info(TAG, "浇水 %d 秒", seconds);

        Board& board = Board::GetInstance();
        std::shared_ptr<Actuator> actuator_ptr = board.GetActuator(kPumpControlName);
        if (actuator_ptr == nullptr) {
            Log::Error(TAG, "未找到电机驱动模块对象。");
            return;
        }

        std::shared_ptr<SingleMotorDriver> pump_control_ptr = std::static_pointer_cast<SingleMotorDriver>(actuator_ptr);
        pump_control_ptr->On(192);
        vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
        pump_control_ptr->Off();
    });
}


void WateringApplication::ShowMessage(const std::string& message) {
    last_message_ = message;

    U8g2Display* display = static_cast<U8g2Display*>( Board::GetInstance().GetDisplay() );
    display->GetWindow()->SetText(3, last_message_);
}
