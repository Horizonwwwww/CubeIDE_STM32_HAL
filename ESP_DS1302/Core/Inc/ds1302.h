#ifndef __DS1302_H
#define __DS1302_H

#include "stm32f1xx_hal.h"

// 引脚定义
#define DS1302_RST_PORT    GPIOA
#define DS1302_RST_PIN     GPIO_PIN_6

#define DS1302_CLK_PORT    GPIOA
#define DS1302_CLK_PIN     GPIO_PIN_5

#define DS1302_DAT_PORT    GPIOA
#define DS1302_DAT_PIN     GPIO_PIN_4

// 时间结构体
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t weekday;
} DS1302_Time;

// 函数声明
void DS1302_Init(void);

void DS1302_ForceInitTime(void);

void DS1302_SetTime(DS1302_Time *time);
void DS1302_GetTime(DS1302_Time *time);
uint8_t BCD2DEC(uint8_t bcd);
uint8_t DEC2BCD(uint8_t dec);

#endif
