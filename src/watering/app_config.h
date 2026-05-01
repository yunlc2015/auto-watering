#ifndef _APP_CONFIG_H
#define _APP_CONFIG_H

//////////////////////////////////////////////////////////////////////////////////
// 特性

#define PLATFORM_API_BASE                   "http://api.yunlc.com.cn/v1"
#define PLATFORM_APP_ID                     "c0ce54cdb7bc4b4bb118cdfec4548f7f"
#define PRODUCT_MODEL                       "esp32-mimi"

// 定时器
#define CONFIG_USE_SW_TIMER                 1

// WIFI配置
#define CONFIG_USE_WIFI                     1
#define CONFIG_WIFI_CONFIGURE_ENABLE        1  

// 时钟（刷新界面）
#define CONFIG_CLOCK_ENABLE                 0  

// OTA
#define CONFIG_OTA_ENABLE                   0  
#define CONFIG_OTA_URL                      ""


#endif //_APP_CONFIG_H
