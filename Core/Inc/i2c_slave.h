/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c_slave.h
  * @brief   I2C slave register-based interface for sensor data and configuration
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef INC_I2C_SLAVE_H_
#define INC_I2C_SLAVE_H_

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define I2C_SLAVE_ADDR   0x38
#define I2C_SLAVE_SW_VER 0x0100

/*
 * Register map exposed over I2C (base offset 0, little-endian).
 * __attribute__((packed)) removes padding; the field layout is kept
 * naturally aligned so no unaligned-access faults occur on Cortex-M3.
 */
typedef struct __attribute__((packed)) {
  uint16_t sw_ver;          /* offset 0x00, 2 bytes, READ-ONLY  */
  uint16_t tacho_timeout;   /* offset 0x02, 2 bytes, READ/WRITE */
  uint32_t tacho_period;    /* offset 0x04, 4 bytes, READ-ONLY  */
  uint16_t adc_values[2];   /* offset 0x08, 4 bytes, READ-ONLY  */
  uint8_t  digi_inputs;     /* offset 0x0C, 1 byte,  READ-ONLY  */
} tacho_i2c_data_t;

void              i2c_slave_init(I2C_HandleTypeDef *hi2c);
tacho_i2c_data_t *i2c_slave_get_data(void);

#endif /* INC_I2C_SLAVE_H_ */
