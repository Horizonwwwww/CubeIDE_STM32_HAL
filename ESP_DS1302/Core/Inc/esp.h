#ifndef __ESP_H
#define __ESP_H

#include "stm32f1xx_hal.h"

#define MAX_DISPLAY_LINES 5
#define MAX_LINE_LENGTH 32

typedef enum {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    MQTT_CONNECTING,
    MQTT_CONNECTED,
    WIFI_ERROR
} WiFi_StatusTypeDef;


void ESP8266_SendCommand(const char *cmd);
// 初始化函数
void ESP8266_Init(UART_HandleTypeDef *huart);
void ESP8266_ConnectToWiFi(const char *ssid, const char *pass);
void ESP8266_ConnectToHuaweiCloud(void);

// 数据发送函数
void ESP8266_SendSensorData(int temp_int, int temp_dec, int hum_int, int hum_dec);
void ESP8266_response(const char* request_id);

//void ESP8266_ProcessCloudCommands(UART_HandleTypeDef *huart, uint32_t timeout);
//void Parse_Cloud_Command_From_Buffer(void);

#endif


