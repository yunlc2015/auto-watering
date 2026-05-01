/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */
#ifndef _ESP32_MINI_BOARD_H
#define _ESP32_MINI_BOARD_H

#include <driver/gpio.h>

#include "src/framework/sys/log.h"
#include "src/framework/app/application.h"
#include "src/framework/board/wifi_board.h"
#include "src/framework/display/display.h"
#include "src/framework/led/led.h"
#include "src/framework/wifi/wifi_configuration.h"

#include "board_config.h"

static const std::string kManualButton      = "manual_button";
static const std::string kPumpControlName   = "pump_control";
static const std::string kSoilMositureName  = "soil_mositure";

class ESP32_MINI_BOARD : public WifiBoard {
private:
    Display *display_ = nullptr;
    WifiConfiguration *wifi_conf_;

    void InitializeDisplay();
    void InitializeButtons();
    void InitializePeripherals();

public:
    ESP32_MINI_BOARD();

    Display* GetDisplay() override { return display_; }

    WifiConfiguration* GetWifiConfiguration() override {return wifi_conf_; }

    void ButtonTick();
};

#endif //_ESP32_MINI_BOARD_H
