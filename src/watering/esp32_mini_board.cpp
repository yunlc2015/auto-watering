/**
 * 物联网自动浇花应用
 * 
 * 本程序可不受限制的用于学习，商业用途请联系作者。
 * 
 * 产品链接：https://www.xpstem.com/product/auto-watering
 * Author: Billy Zhang（vx: billyzh）
 */
#include "esp32_mini_board.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <esp_system.h>

#include "src/framework/board/onebutton_impl.h"
#include "src/framework/display/u8g2_display.h"
#include "src/framework/sys/system_reset.h"
#include "src/framework/led/gpio_led.h"
#include "src/framework/peripheral/sensor.h"
#include "src/framework/peripheral/sensor_value.h"
#include "src/framework/wifi/wifi_station.h"
#include "src/framework/peripheral/single_motor_driver.h"
#include "wifi_configuration_ex.h"

#define TAG "ESP32_MINI_BOARD"

void* create_board() { 
    return new ESP32_MINI_BOARD();
}

ESP32_MINI_BOARD::ESP32_MINI_BOARD() : WifiBoard() {

    Log::Info(TAG, "===== Create Board ...... =====");

    InitializeDisplay();

    InitializeButtons();

    InitializePeripherals();

    wifi_conf_ = new WifiConfigurationEx();
    
    Log::Info( TAG, "===== Board config completed. =====");
}

void ESP32_MINI_BOARD::InitializeDisplay() {

    Log::Info( TAG, "Init ssd1306 display ......" );
    U8G2 *u8g2 = new U8G2_SSD1306_128X64_NONAME_F_SW_I2C(
        /* rotation */ U8G2_R0, 
        /* i2c clk */ I2C_SCL_PIN,
        /* i2c data */ I2C_SDA_PIN,
        /* reset=*/ U8X8_PIN_NONE
    );
    u8g2->setI2CAddress(OLED_I2C_ADDRESS << 1);

    //u8g2_font_unifont_t_chinese2
    display_ = new U8g2Display(u8g2, DISPLAY_WIDTH, DISPLAY_HEIGHT, u8g2_font_wqy14_t_gb2312);

}

void ESP32_MINI_BOARD::ButtonTick() {
    for (const auto& pair : button_map()) {
        pair.second->Tick();
    }
}

void ESP32_MINI_BOARD::InitializeButtons() {
    Log::Info( TAG, "Init buttons ......");

    std::shared_ptr<Button> button1 = std::make_shared<OneButtonImpl>(kBootButton, BOOT_BUTTON_PIN);
    button1->SetLongPressIntervalMs(1000);
    button1->BindAction(ButtonAction::LongPress);
    AddButton(button1);

    std::shared_ptr<Button> button2 = std::make_shared<OneButtonImpl>(kManualButton, MANUAL_BUTTON_PIN);
    button2->BindAction(ButtonAction::DoubleClick);
    AddButton(button2);
}

void ESP32_MINI_BOARD::InitializePeripherals() {
    
    Log::Info( TAG, "Init peripherals ......");

    std::shared_ptr<AnalogSensor> soil_moisture_ptr = std::make_shared<AnalogSensor>(kSoilMositureName, SOIL_MOISTURE_PIN);
    AddSensor(soil_moisture_ptr);

    std::shared_ptr<SingleMotorDriver> pump_control_ptr = std::make_shared<SingleMotorDriver>(L9110_PIN_A, L9110_PIN_B);
    AddActuator(kPumpControlName, pump_control_ptr);

}
