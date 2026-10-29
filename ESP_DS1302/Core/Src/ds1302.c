#include "ds1302.h"
#include "main.h"

// 私有函数
static void DS1302_WriteByte(uint8_t dat);
static uint8_t DS1302_ReadByte(void);
static void DS1302_Set_DAT_Input(void);
static void DS1302_Set_DAT_Output(void);

void DS1302_Init(void) {
    // 初始化GPIO已配置
    HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_RESET);


}

void DS1302_ForceInitTime(void) {
    DS1302_Time initTime = {
        .seconds = 0,
        .minutes = 14,
        .hours = 21,
        .day = 9,
        .month = 8,
        .year = 25,   // 代表2025年
        .weekday = 2
    };
    DS1302_SetTime(&initTime);
}

void DS1302_SetTime(DS1302_Time *time) {
    // 停止时钟（设置CH位为1）
    HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_SET);
    DS1302_WriteByte(0x8E); // 控制寄存器地址
    DS1302_WriteByte(0x00); // 允许写入
    HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_RESET);

    // 写入时间
    HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_SET);
    DS1302_WriteByte(0xBE); // 突发写入模式

    DS1302_WriteByte(DEC2BCD(time->seconds) & 0x7F); // 秒，清除CH位
    DS1302_WriteByte(DEC2BCD(time->minutes)); // 分
    DS1302_WriteByte(DEC2BCD(time->hours));   // 时
    DS1302_WriteByte(DEC2BCD(time->day));     // 日
    DS1302_WriteByte(DEC2BCD(time->month));   // 月
    DS1302_WriteByte(DEC2BCD(time->weekday)); // 星期
    DS1302_WriteByte(DEC2BCD(time->year));    // 年
    DS1302_WriteByte(0x00); // 控制字节

    HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_RESET);
}

void DS1302_GetTime(DS1302_Time *time) {
    HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_SET);
    DS1302_WriteByte(0xBF); // 突发读取模式

    time->seconds = BCD2DEC(DS1302_ReadByte() & 0x7F);
    time->minutes = BCD2DEC(DS1302_ReadByte());
    time->hours = BCD2DEC(DS1302_ReadByte());
    time->day = BCD2DEC(DS1302_ReadByte());
    time->month = BCD2DEC(DS1302_ReadByte());
    time->weekday = BCD2DEC(DS1302_ReadByte());
    time->year = BCD2DEC(DS1302_ReadByte());

    HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_RESET);
}

// 私有函数实现
static void DS1302_WriteByte(uint8_t dat) {
    DS1302_Set_DAT_Output();
    for(uint8_t i=0; i<8; i++) {
        HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(DS1302_DAT_PORT, DS1302_DAT_PIN, (dat & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_SET);
        dat >>= 1;
    }
}

static uint8_t DS1302_ReadByte(void) {
    uint8_t dat = 0;
    DS1302_Set_DAT_Input();
    for(uint8_t i=0; i<8; i++) {
        dat >>= 1;
        HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_RESET);
        if(HAL_GPIO_ReadPin(DS1302_DAT_PORT, DS1302_DAT_PIN)) dat |= 0x80;
        HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_SET);
    }
    return dat;
}

static void DS1302_Set_DAT_Input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_DAT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS1302_DAT_PORT, &GPIO_InitStruct);
}

static void DS1302_Set_DAT_Output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_DAT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1302_DAT_PORT, &GPIO_InitStruct);
}

uint8_t BCD2DEC(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t DEC2BCD(uint8_t dec) {
    return ((dec / 10) << 4) + (dec % 10);
}

