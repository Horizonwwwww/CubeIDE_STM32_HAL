#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

#define DHT11_PORT GPIOA
#define DHT11_PIN  GPIO_PIN_1

typedef struct {
    uint8_t humidity_int;
    uint8_t humidity_decimal;
    uint8_t temp_int;
    uint8_t temp_decimal;
    uint8_t checksum;
    uint8_t valid;
} DHT11_Data;

uint8_t DHT11_Read(DHT11_Data *data);

#endif
