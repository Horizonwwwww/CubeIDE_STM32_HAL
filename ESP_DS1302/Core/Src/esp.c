#include "esp.h"
#include "oled.h"
#include "font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "usart.h"


/*
【密钥部分已做删减！！！】
#define DEVICE_ID ""
#define DEVICE_SECRET ""
#define ClientID ""

#define HUAWEI_CLOUD_SERVER ""
#define HUAWEI_CLOUD_PORT 1883
*/
// 华为云IoT平台配置
#define DEVICE_ID "68.........8_MQTT"
#define DEVICE_SECRET "1a........449"
#define ClientID "68........03"

#define HUAWEI_CLOUD_SERVER "82.......cloud.com"
#define HUAWEI_CLOUD_PORT 1883

UART_HandleTypeDef *espUartHandle = NULL;
WiFi_StatusTypeDef wifiStatus = WIFI_DISCONNECTED;

char mqtt_cmd_buffer[1024];  // 使用 .bss 或 .data 区，避免栈溢出

void ESP8266_SendCommand(const char *cmd) {
    HAL_UART_Transmit(espUartHandle, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
    HAL_UART_Transmit(espUartHandle, (uint8_t *)"\r\n", 2, HAL_MAX_DELAY);
}


void ESP8266_Init(UART_HandleTypeDef *huart) {
    espUartHandle = huart;

    ESP8266_SendCommand("AT");
    // Reset ESP8266
    HAL_Delay(1500);
    ESP8266_SendCommand("AT+RST");
    HAL_Delay(1000);
    ESP8266_SendCommand("ATE0");
    HAL_Delay(500);
    ESP8266_SendCommand("AT");
    HAL_Delay(500);
}

void ESP8266_ConnectToWiFi(const char *ssid, const char *pass) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pass);
    ESP8266_SendCommand(cmd);
    wifiStatus = WIFI_CONNECTING;
    HAL_Delay(5000); // 等待WiFi连接
}


void ESP8266_ConnectToHuaweiCloud(void) {
    char cmd[512];

    // 配置MQTT用户信息
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"",
             DEVICE_ID, DEVICE_SECRET);
    ESP8266_SendCommand( cmd);
    HAL_Delay(1000);

    // 配置MQTT客户端ID
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTCLIENTID=0,\"%s\"",
			 ClientID);
    ESP8266_SendCommand(cmd);
    HAL_Delay(1000);

    // 连接MQTT服务器
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTCONN=0,\"%s\",%d,1",
             HUAWEI_CLOUD_SERVER, HUAWEI_CLOUD_PORT);
    ESP8266_SendCommand(cmd);
    wifiStatus = MQTT_CONNECTING;
    HAL_Delay(1000);

    // 订阅主题——属性
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTSUB=0,\"$oc/devices/%s/sys/properties/report\",1",
             DEVICE_ID);
    ESP8266_SendCommand(cmd);
    HAL_Delay(2000);

    // 订阅主题——命令
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTSUB=0,\"$oc/devices/%s/sys/commands/#\",1",
             DEVICE_ID);
    ESP8266_SendCommand(cmd);
    HAL_Delay(2000);

    // 订阅主题——命令响应
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTSUB=0,\"$oc/devices/%s/sys/commands/response/#\",1",
             DEVICE_ID);
    ESP8266_SendCommand(cmd);
    HAL_Delay(2000);

}

void ESP8266_SendSensorData(int temp_int, int temp_dec, int hum_int, int hum_dec)
{


    // 构建带转义的 JSON payload 直接写入 mqtt_cmd_buffer
    snprintf(mqtt_cmd_buffer, sizeof(mqtt_cmd_buffer),
             "AT+MQTTPUB=0,\"$oc/devices/%s/sys/properties/report\","
             "\"{\\\"services\\\":[{\\\"service_id\\\":\\\"LK\\\"\\,\\\"properties\\\":{\\\"DHT11_T\\\":%d.%d\\,\\\"DHT11_H\\\":%d.%d}}]}\",0,0",
             DEVICE_ID, temp_int, temp_dec, hum_int, hum_dec);


    ESP8266_SendCommand(mqtt_cmd_buffer);
    HAL_UART_Transmit_DMA(&huart2, (uint8_t*)"SendSensorData done\r\n", 22);

    HAL_Delay(2000);
}

void ESP8266_response(const char* requestid)
{
	                            // 发送响应（所有命令共用相同响应格式）
	                            snprintf(mqtt_cmd_buffer, sizeof(mqtt_cmd_buffer),
	                            		"AT+MQTTPUB=0,\"$oc/devices/%s/sys/commands/response/request_id=%s\",\"\",0,0",
	                                    DEVICE_ID, requestid);

	                            ESP8266_SendCommand(mqtt_cmd_buffer);



}





