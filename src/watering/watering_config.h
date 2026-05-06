/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */
#ifndef _WATERING_CONFIG_H
#define _WATERING_CONFIG_H

#include <string>
#include <ArduinoJson.h>
#include "src/framework/sys/settings.h"
#include "src/framework/sys/log.h"

#define TAG "WateringConfig"

class WateringConfig {
public:
    static WateringConfig& GetInstance() {
        static WateringConfig instance;
        return instance;
    }

    const std::string& project_id() const { return project_id_; }
    const std::string& mqtt_server() const { return mqtt_server_; }
    const int mqtt_port() const { return mqtt_port_; }
    const std::string& mqtt_username() const { return mqtt_username_; }
    const std::string& mqtt_password() const { return mqtt_password_; }
    const std::string& pump_control_topic() const { return pump_control_topic_; }
    const std::string& soil_moisture_topic() const { return soil_moisture_topic_; }
    const std::string& soil_moisture_dataname() const { return soil_moisture_dataname_; }

    void Update(const std::string& platform_token, const std::string& serialno, int workmode, const JsonObject& data_node) {

        int version = data_node["version"];
        std::string project_id = data_node["projectId"].as<const char*>();

        //if (version==1) {
            // peripherals
            JsonObject peri_node = data_node["peripherals"].as<JsonObject>();

            std::string soil_moisture_id = peri_node["soilMoistureSensor"]["id"].as<const char*>();
            std::string soil_moisture_dataname = peri_node["soilMoistureSensor"]["config"]["dataname"].as<const char*>();
            std::string soil_moisture_topic = std::string("user/") + soil_moisture_id + std::string("/data");

            std::string pump_control_id = peri_node["pumpControlRelay"]["id"].as<const char*>();
            std::string pump_control_topic = std::string("user/") + pump_control_id + std::string("/ctrl");
        //}

        // iot
        JsonObject iot_node = data_node["iot"].as<JsonObject>();
        std::string mqtt_server = iot_node["mqttServer"].as<const char*>();
        int mqtt_port = iot_node["mqttPort"];
        std::string mqtt_username = iot_node["mqttUsername"].as<const char*>();
        std::string mqtt_password = iot_node["mqttPassword"].as<const char*>();
        
        Settings settings("config", true);
        settings.SetString("plfm_token", platform_token);
        settings.SetString("serialno", serialno);
        settings.SetInt("workmode", workmode);
        settings.SetString("projectid", project_id);
        settings.SetString("sm_topic", soil_moisture_topic);
        settings.SetString("sm_dataname", soil_moisture_dataname);
        settings.SetString("pc_topic", pump_control_topic);
        settings.SetString("mq_server",mqtt_server);
        settings.SetInt("mq_port", mqtt_port);
        settings.SetString("mq_username", mqtt_username);
        settings.SetString("mq_password", mqtt_password);
    }
    
    void Reset() {
        Settings settings("config", true);
        settings.EraseAll();
    }

private:
    WateringConfig() {
        Settings settings("config");
        std::string serialno = settings.GetString("serialno", "");
        if (serialno == "") {
            Log::Warn(TAG, "no configdata");
            return;
        }

        platform_token_         = settings.GetString("plfm_token");
        project_id_             = settings.GetString("projectid" );
        soil_moisture_topic_    = settings.GetString("sm_topic" );
        soil_moisture_dataname_ = settings.GetString("sm_dataname" );
        pump_control_topic_     = settings.GetString("pc_topic" );
        mqtt_server_            = settings.GetString("mq_server", "iot.yunlc.com.cn" );
        mqtt_port_              = settings.GetInt("mq_port", 1883 );
        mqtt_username_          = settings.GetString("mq_username" );
        mqtt_password_          = settings.GetString("mq_password" );

        Log::Debug(TAG, "platform_token: %s", platform_token_.c_str());
        Log::Debug(TAG, "project_id: %s", project_id_.c_str());
        Log::Debug(TAG, "soil_moisture_topic: %s", soil_moisture_topic_.c_str());
        Log::Debug(TAG, "soil_moisture_dataname: %s", soil_moisture_dataname_.c_str());
        Log::Debug(TAG, "pump_control_topic: %s", pump_control_topic_.c_str());
        Log::Debug(TAG, "mqtt_server: %s", mqtt_server_.c_str());
        Log::Debug(TAG, "mqtt_username: %s", mqtt_username_.c_str());
        Log::Debug(TAG, "mqtt_password: %s", mqtt_password_.c_str());
    }

    std::string platform_token_;
    std::string project_id_;
    std::string mqtt_server_;
    int mqtt_port_;
    std::string mqtt_username_;
    std::string mqtt_password_;
    std::string pump_control_topic_;
    std::string soil_moisture_topic_;
    std::string soil_moisture_dataname_;

};

#endif //_WATERING_CONFIG_H