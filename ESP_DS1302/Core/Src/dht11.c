#include "dht11.h"
#include "main.h"

// 使用DWT(Data Watchpoint and Trace)实现微秒延时，不需要定时器
#define DWT_CYCCNT ((volatile uint32_t *)0xE0001004)
#define DWT_CONTROL ((volatile uint32_t *)0xE0001000)
#define SCB_DEMCR ((volatile uint32_t *)0xE000EDFC)

#define DHT11_TIMEOUT 10000 // 10ms超时

static void DWT_Init(void)
{
    *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
    *DWT_CYCCNT = 0;
    *DWT_CONTROL = *DWT_CONTROL | 1;
}

static void DHT11_Delay_us(uint32_t us)
{
    uint32_t start = *DWT_CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000);
    while ((*DWT_CYCCNT - start) < cycles);
}

static void DHT11_Set_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void DHT11_Set_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void DHT11_Start(void)
{
    DHT11_Set_Output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20); // 保持低电平至少18ms

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    DHT11_Delay_us(30); // 拉高30us

    DHT11_Set_Input();
}

static uint8_t DHT11_Check_Response(void)
{
    uint32_t timeout = 0;

    // 等待DHT11拉低总线
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
    {
        timeout++;
        if (timeout > DHT11_TIMEOUT)
            return 0;
    }

    timeout = 0;
    // 等待DHT11拉高总线
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET)
    {
        timeout++;
        if (timeout > DHT11_TIMEOUT)
            return 0;
    }

    timeout = 0;
    // 等待DHT11再次拉低总线
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
    {
        timeout++;
        if (timeout > DHT11_TIMEOUT)
            return 0;
    }

    return 1;
}

static uint8_t DHT11_Read_Byte(void)
{
    uint8_t data = 0;

    for (int i = 0; i < 8; i++)
    {
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET);

        DHT11_Delay_us(40);

        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
        {
            data |= (1 << (7 - i));
            while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET);
        }
    }

    return data;
}

uint8_t DHT11_Read(DHT11_Data *data)
{
    uint8_t buf[5] = {0};

    // 初始化DWT计数器
    DWT_Init();

    DHT11_Start();

    if (!DHT11_Check_Response())
    {
        data->valid = 0;
        return 0;
    }

    for (int i = 0; i < 5; i++)
    {
        buf[i] = DHT11_Read_Byte();
    }

    if (buf[0] + buf[1] + buf[2] + buf[3] == buf[4])
    {
        data->humidity_int = buf[0];
        data->humidity_decimal = buf[1];
        data->temp_int = buf[2];
        data->temp_decimal = buf[3];
        data->checksum = buf[4];
        data->valid = 1;
        return 1;
    }

    data->valid = 0;
    return 0;
}
