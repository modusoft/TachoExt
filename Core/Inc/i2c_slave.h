#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include "main.h"
#include <stdint.h>

#define I2C_SLAVE_ADDR    0x38
#define I2C_SLAVE_SW_VER  0x0100

typedef struct __attribute__((packed)) {
  uint16_t sw_ver;
  uint16_t tacho_timeout;
  uint32_t tacho_period;
  uint16_t adc_values[2];
  uint8_t  digi_inputs;
} tacho_i2c_data_t;

void i2c_slave_init(I2C_HandleTypeDef *hi2c);
tacho_i2c_data_t *i2c_slave_get_data(void);
void i2c_slave_ev_irq(void);
void i2c_slave_er_irq(void);

void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);

#endif /* I2C_SLAVE_H */
