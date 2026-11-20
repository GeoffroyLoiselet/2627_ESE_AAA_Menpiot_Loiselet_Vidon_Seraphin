#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include "main.h"
#include <stdint.h>

/* Number of LEDs on the PB4 chain */
#define NEO_NUM_LEDS  1

void            Neo_Init(void);
void            Neo_SetPixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
void            Neo_SetAll(uint8_t r, uint8_t g, uint8_t b);
void            Neo_Wheel(uint16_t pos, uint8_t brightness, uint8_t *r, uint8_t *g, uint8_t *b);
HAL_StatusTypeDef Neo_Show(void);   /* non-blocking, returns HAL_BUSY if previous frame still in flight */
uint8_t         Neo_IsBusy(void);

#endif
