/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */
#ifndef _WIFI_CONFIGURATION_EX_H
#define _WIFI_CONFIGURATION_EX_H

#include <Arduino.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "src/framework/wifi/wifi_configuration.h"

class WifiConfigurationEx : public WifiConfiguration {
public:
    WifiConfigurationEx() { }

protected:
    void StartWebServer() override;

private:
    bool ActivateProduct(const std::string& platform_token, const std::string& serialno, int workmode);

    WebServer *web_server_;
    TaskHandle_t web_task_handler_;

};

#endif //_WIFI_CONFIGURATION_EX_H
