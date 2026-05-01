/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */
#ifndef _MQTT_SERVICE_H
#define _MQTT_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>
#include <map>
#include <string>
#include "src/framework/sys/frt_task.h"

class MqttService {
public:
    MqttService();
    ~MqttService();
    int Connect(const std::string& broker_server, const std::string& username, const std::string& password, uint8_t retry_count);
    void PublishData(const std::string& topic, const std::string& payload);
    void SubscribeTopic(const std::string& topic, std::function<void(const std::string&)> callback);
    void Disconnect();

private:
    void OnMessage(char *mqtt_topic, byte *payload, unsigned int length);

    WiFiClient wifi_client_;
    PubSubClient *mqtt_client_;
    FrtTask* mqtt_task_;
    std::map<std::string, std::function<void(const std::string&)>> subscribe_callback_;

};

#endif //_MQTT_SERVICE_H