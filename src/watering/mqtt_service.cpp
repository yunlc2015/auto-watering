/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */
#include "mqtt_service.h"
#include <esp_system.h>

#include "src/framework/sys/log.h"
#include "src/framework/sys/system_info.h"
#include "src/framework/board/board.h"
#include "watering_application.h"

#define TAG "MqttService"

MqttService::MqttService() {
    mqtt_client_ = new PubSubClient(wifi_client_);
    mqtt_client_->setKeepAlive(60);
    mqtt_client_->setCallback([this](char *mqtt_topic, byte *payload, unsigned int length) {
        OnMessage(mqtt_topic, payload, length);
    });
}

MqttService::~MqttService() {
    if (mqtt_task_!=nullptr) {
        mqtt_task_->Stop();
    }

    if (mqtt_client_!=nullptr) {
        mqtt_client_->disconnect();
    }
}

int MqttService::Connect(const std::string& broker_server, const std::string& username, const std::string& password, uint8_t retry_count) {
    
    mqtt_client_->setServer(broker_server.c_str(), 1883);

    std::string client_id = "esp32-" + SystemInfo::GetMacAddress2();
    Log::Info(TAG, "连接IoT平台: %s, username: %s", broker_server.c_str(), username.c_str());
    while (!mqtt_client_->connected() && retry_count > 0) {
        if (mqtt_client_->connect(client_id.c_str(), username.c_str(), password.c_str())) {
            Log::Info(TAG, "连接到IoT平台.");
            return 0;
        }
        Log::Info(TAG, "连接失败, rc=%d", mqtt_client_->state());

        retry_count --;
        Log::Info(TAG, "3秒后重试。");
        delay(3000);
    } 

    return mqtt_client_->state();
}

void MqttService::Disconnect() {
    if (mqtt_client_!=nullptr) {
        mqtt_client_->disconnect();
    }
}

void MqttService::PublishData(const std::string& topic, const std::string& payload) {
    mqtt_client_->publish(topic.c_str(), payload.c_str());
}

void MqttService::SubscribeTopic(const std::string& mqtt_topic, std::function<void(const std::string&)> callback) {
    subscribe_callback_[mqtt_topic] = callback;
    mqtt_client_->subscribe(mqtt_topic.c_str());

    if (mqtt_task_ == nullptr) {
        mqtt_task_ = new FrtTask("Mqtt_LoopTask");
        mqtt_task_->OnLoop([this]() {
            mqtt_client_->loop();
        });
        mqtt_task_->Start(4096, tskIDLE_PRIORITY + 1);
    }
}

void MqttService::OnMessage(char *mqtt_topic, byte *payload, unsigned int length) {
    if (length < 1) {
      return;
    }

    char data[length];
    for (unsigned int i = 0; i < length; i++) {
        data[i] = (char) payload[i];
    }
    
    std::string topic = mqtt_topic;
    std::string payload_str = data;

    auto it = subscribe_callback_.find(mqtt_topic);
    if (it != subscribe_callback_.end()) {
        it->second(payload_str);
    }

}