#include "i2c_slave.h"
#include <string.h>

#define TACHO_TIMEOUT_OFFSET_LO  0x02u
#define TACHO_TIMEOUT_OFFSET_HI  0x03u

static tacho_i2c_data_t i2c_data;
static uint8_t i2c_rd_buf[sizeof(tacho_i2c_data_t)];
static uint8_t reg_ptr;
static I2C_HandleTypeDef *i2c_handle;
static volatile uint8_t got_reg_addr;
static volatile uint8_t is_transmitting;

void i2c_slave_init(I2C_HandleTypeDef *hi2c)
{
  i2c_handle = hi2c;

  i2c_data.sw_ver = I2C_SLAVE_SW_VER;

  hi2c->Instance->CR1 |= I2C_CR1_ACK | I2C_CR1_PE;
  hi2c->Instance->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN;
}

tacho_i2c_data_t *i2c_slave_get_data(void)
{
  return &i2c_data;
}

static void i2c_slave_update_data(void)
{
  i2c_data.digi_inputs = (uint8_t)(~GPIOA->IDR & 0xFFu);
}

void I2C1_EV_IRQHandler(void)
{
  i2c_slave_ev_irq();
}

void I2C1_ER_IRQHandler(void)
{
  i2c_slave_er_irq();
}

void i2c_slave_ev_irq(void)
{
  I2C_TypeDef *i2c = i2c_handle->Instance;
  uint8_t struct_size = (uint8_t)sizeof(tacho_i2c_data_t);
  uint32_t sr1 = i2c->SR1;

  /* 1. ADDR: address matched */
  if (sr1 & I2C_SR1_ADDR)
  {
    uint32_t sr2 = i2c->SR2; /* Reading SR2 clears ADDR flag */
    if (sr2 & I2C_SR2_TRA)
    {
      /* Slave transmitter: master wants to read from us */
      if (got_reg_addr == 0u)
      {
        i2c_slave_update_data();
        memcpy(i2c_rd_buf, &i2c_data, sizeof(tacho_i2c_data_t));
      }
      if (reg_ptr >= struct_size)
      {
        reg_ptr = 0u;
      }
      i2c->DR = i2c_rd_buf[reg_ptr];
      reg_ptr = (uint8_t)((reg_ptr + 1u) % struct_size);
      is_transmitting = 1u;
    }
    else
    {
      /* Slave receiver: master wants to write to us */
      is_transmitting = 0u;
      got_reg_addr = 0u;
    }
    i2c->CR1 |= I2C_CR1_ACK;
    return;
  }

  /* 2. TXE: transmit buffer empty (slave TX mode) */
  if ((sr1 & I2C_SR1_TXE) && (is_transmitting != 0u))
  {
    i2c->DR = i2c_rd_buf[reg_ptr];
    reg_ptr = (uint8_t)((reg_ptr + 1u) % struct_size);
    return;
  }

  /* 3. RXNE: receive buffer not empty (slave RX mode) */
  if (sr1 & I2C_SR1_RXNE)
  {
    uint8_t data = (uint8_t)i2c->DR;
    if (got_reg_addr == 0u)
    {
      reg_ptr = (data < struct_size) ? data : 0u;
      i2c_slave_update_data();
      memcpy(i2c_rd_buf, &i2c_data, sizeof(tacho_i2c_data_t));
      got_reg_addr = 1u;
    }
    else
    {
      uint8_t target = (uint8_t)(reg_ptr % struct_size);
      if ((target >= TACHO_TIMEOUT_OFFSET_LO) && (target <= TACHO_TIMEOUT_OFFSET_HI))
      {
        ((uint8_t *)&i2c_data)[target] = data;
      }
      reg_ptr = (uint8_t)((target + 1u) % struct_size);
    }
    return;
  }

  /* 4. STOPF: stop condition detected */
  if (sr1 & I2C_SR1_STOPF)
  {
    i2c->CR1 |= I2C_CR1_PE;
    got_reg_addr = 0u;
    is_transmitting = 0u;
    return;
  }

  /* 5. AF: acknowledge failure (master sent NACK) */
  if (sr1 & I2C_SR1_AF)
  {
    i2c->SR1 &= ~I2C_SR1_AF;
    got_reg_addr = 0u;
    is_transmitting = 0u;
    return;
  }
}

void i2c_slave_er_irq(void)
{
  I2C_TypeDef *i2c = i2c_handle->Instance;

  i2c->SR1 &= ~(I2C_SR1_BERR | I2C_SR1_OVR | I2C_SR1_AF | I2C_SR1_ARLO);

  got_reg_addr = 0u;
  is_transmitting = 0u;

  i2c->CR1 |= I2C_CR1_ACK | I2C_CR1_PE;
}
