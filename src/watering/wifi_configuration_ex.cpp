/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */
#include "wifi_configuration_ex.h"

#include <nvs.h>
#include <nvs_flash.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "src/framework/sys/log.h"
#include "src/framework/sys/settings.h"
#include "src/framework/wifi/ssid_manager.h"
#include "src/framework/sys/system_info.h"
#include "wifi_configuration_res.h"
#include "watering_config.h"
#include "watering_application.h"

#define TAG "WifiConfigurationEx"

void _webServerTask(void *pvParam) {
    WebServer* server = (WebServer*)pvParam;
    while (1) {
        server->handleClient();
        delay(10);
    }
}

void _rebootTask(void* pvParam) {
    // 等待200ms确保HTTP响应完全发送
    vTaskDelay(pdMS_TO_TICKS(200));
    // 停止Web服务器
    WebServer* server = (WebServer*)pvParam;
    if (server!=nullptr) {
        server->stop();
    }
    // 再等待100ms确保所有连接都已关闭
    vTaskDelay(pdMS_TO_TICKS(100));
    // 执行重启
    esp_restart();
}

void WifiConfigurationEx::StartWebServer() {

    web_server_ = new WebServer(80);

    // GET / 
    web_server_->on("/", [this](){ 
        web_server_->send(200, "text/html", index_html); 
    });

    // GET / 
    web_server_->on("/mini.css", [this](){ 
        web_server_->send(200, "text/css", mini_css); 
    });

    // GET / 
    web_server_->on("/zepto.js", [this](){ 
        web_server_->send(200, "text/javascript", zepto_js); 
    });

    // POST /submit --json
    web_server_->on("/submit", [this](){ 
        
        // post参数，form表单
        std::string ssid = std::string(web_server_->arg("ssid").c_str());
        std::string password = std::string(web_server_->arg("password").c_str());
        std::string serialno = std::string(web_server_->arg("serialno").c_str());  //序列号
        std::string platform_token = std::string(web_server_->arg("platform_token").c_str());  //序列号
        int workmode = web_server_->arg("workmode").toInt();     //工作模式

        if (ssid=="" || password=="" || serialno=="" || platform_token=="") {
            web_server_->send(200, "application/json", "{\"success\":false,\"error\":\"参数不能为空\"}");
            return;
        }

        Log::Info(TAG, "ssid: %s, password: %s", ssid.c_str(), password.c_str());
        if (!this->ConnectToWifi(ssid, password)) {
            web_server_->send(200, "application/json", "{\"success\":false,\"error\":\"无法连接到 WiFi\"}");
            return;
        }

        bool status = ActivateProduct(platform_token, serialno, workmode);
        WiFi.disconnect();

        if (!status) {
            web_server_->send(200, "application/json", "{\"success\":false,\"error\":\"获取产品配置失败！\"}");
            return;
        }

        Log::Info(TAG, "Save SSID %s %d", ssid.c_str(), ssid.length());
        SsidManager::GetInstance().AddSsid(ssid, password);

        web_server_->send(200, "application/json", "{\"success\":true}"); 

    });

        
    // GET /done.html
    web_server_->on("/done.html", [this](){ 
        web_server_->send(200, "text/html", done_html); 
    });
    
    // POST /reboot
    web_server_->on("/reboot", [this](){ 
        web_server_->send(200, "application/json", "{\"success\":true}"); 

        // 创建一个延迟重启任务
        Log::Info(TAG, "Rebooting..." );
        xTaskCreate(_rebootTask, "reboot_task", 4096, web_server_, 5, NULL);
    });

    web_server_->begin();
    
    xTaskCreate(_webServerTask, "WebServer_Task", 8192, web_server_, 1, &web_task_handler_);

    Log::Info(TAG, "WebServer started.");
}

bool WifiConfigurationEx::ActivateProduct(const std::string& platform_token, const std::string& serialno, int workmode) {
    
    Log::Info(TAG, "activate product, token: %s, serialno: %s", platform_token.c_str(), serialno.c_str());
    
    // 获取项目配置信息
    std::string config_url = PLATFORM_API_BASE "/product/activate";
    Log::Info(TAG, "access: %s", config_url.c_str());

    HTTPClient http;
    http.begin(String(config_url.c_str()));

    http.setAuthorizationType("Bearer");
    http.setAuthorization(platform_token.c_str());

    JsonDocument payloadDoc;
    payloadDoc["appId"] = PLATFORM_APP_ID;
    payloadDoc["model"] = PRODUCT_MODEL;
    payloadDoc["chipId"] = SystemInfo::GetMacAddress2().c_str();
    payloadDoc["serialno"] = serialno.c_str();
    payloadDoc["version"] = Application::GetInstance().GetAppVersion().c_str();
    String payloadStr;
    serializeJson(payloadDoc, payloadStr);

    http.addHeader("Content-Type", "application/json");

    int status_code = http.POST(payloadStr);
    if (status_code != 200) {
        Log::Warn(TAG, "product activate failure, status code: %d", status_code);
        return false;
    }

    String body = http.getString();
    http.end();

    JsonDocument respDoc;
    if (deserializeJson(respDoc, body)) {
        Log::Error(TAG, "解析配置错误");
        return false;
    }

    int err_code = respDoc["errCode"];
    if (err_code!=0) {
        std::string err_msg = respDoc["errMsg"];
        Log::Error(TAG, "获取配置失败，%s", err_msg.c_str());
        return false;
    }

    JsonObject data_node = respDoc["data"].as<JsonObject>();

    WateringConfig& config = WateringConfig::GetInstance();
    config.Update(platform_token, serialno, workmode, data_node);
    
    Log::Info(TAG, "write to ns:config successfully.");

    return true;
}