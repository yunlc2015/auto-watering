/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */

#ifndef _WATERING_APPLICATION_H
#define _WATERING_APPLICATION_H

#include "src/framework/app/application.h"
#include "mqtt_service.h"
#include <string>

class WateringApplication : public Application {
public:
    WateringApplication();
    
    bool OnPhysicalButtonEvent(const std::string& button_name, const ButtonAction action) override;
    bool OnSensorDataEvent(const std::string& sensor_name, const SensorValue& value) override;
    
    void ShowWifiConfigHit(const std::string& ssid, const std::string& config_url, const std::string& mac_address) override;

    const std::string GetAppVersion() const override { return "1.0.0"; }

protected:
    void OnInit() override;
    void OnLoop() override;

private:
    void OnIotMessageEvent(const std::string& topic, const std::string& payload);
    void DoWatering(uint8_t seconds);
    void ShowMessage(const std::string& message);

    MqttService *mqtt_service_ = nullptr;
    int collect_count_ = 0;
    bool started_ = false;
    int soil_moilture_value_ = 0;
    std::string last_message_;

};

#endif //_WATERING_APPLICATION_H
