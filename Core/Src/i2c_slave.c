/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c_slave.c
  * @brief   I2C slave register-based interface implementation
  *
  * Implements a register-map I2C slave at address I2C_SLAVE_ADDR (0x38).
  * Exposes tacho_i2c_data_t as a flat byte-addressable register map.
  * Read atomicity: snapshot on first byte (register address) received.
  * Write atomicity: tacho_timeout committed on last byte (offset 0x03).
  ******************************************************************************
  */
/* USER CODE END Header */

#include "i2c_slave.h"
#include <string.h>

/* Offsets / sizes for tacho_timeout field ---------------------------------*/
#define TACHO_TIMEOUT_OFFSET_FIRST  0x02u
#define TACHO_TIMEOUT_OFFSET_LAST   0x03u

/* Live data accessed by the main loop -------------------------------------*/
static tacho_i2c_data_t i2c_data;

/* Shadow read buffer (snapshot taken at register-address byte) ------------*/
static uint8_t i2c_rd_buf[sizeof(tacho_i2c_data_t)];

/* Staging write buffer (committed on last byte of writable field) ---------*/
static uint8_t i2c_wr_buf[sizeof(tacho_i2c_data_t)];

/* Current register pointer (auto-increments, wraps at struct size) --------*/
static uint8_t reg_ptr;

/* Saved HAL handle --------------------------------------------------------*/
static I2C_HandleTypeDef *i2c_handle;

/* Transaction state -------------------------------------------------------*/
static uint8_t is_write_transaction;  /* 1 when master is writing */
static uint8_t snapshot_done;         /* 1 when i2c_rd_buf is valid */
static uint8_t write_byte_count;      /* bytes received after reg_ptr byte */

/* Single-byte RX/TX staging variables for HAL sequential transfers --------*/
static uint8_t rx_byte;
static uint8_t tx_byte;

/* -------------------------------------------------------------------------*/

/**
 * @brief  Initialize the I2C slave module.
 *         Fixes OwnAddress1 (CubeMX may have generated wrong value) and
 *         starts HAL Listen mode.
 * @param  hi2c  Pointer to the I2C peripheral handle (hi2c1)
 */
void i2c_slave_init(I2C_HandleTypeDef *hi2c)
{
  i2c_handle = hi2c;

  /* Fix OwnAddress1: CubeMX generated 224 (0xE0) instead of 0x70 */
  hi2c->Init.OwnAddress1 = (I2C_SLAVE_ADDR << 1);
  HAL_I2C_Init(hi2c);

  /* Set software version in live data */
  i2c_data.sw_ver = I2C_SLAVE_SW_VER;

  /* Start listening for I2C address match */
  HAL_I2C_EnableListen_IT(hi2c);
}

/**
 * @brief  Return pointer to the live data struct.
 *         The main loop writes tacho_period, adc_values, digi_inputs here.
 * @retval Pointer to tacho_i2c_data_t
 */
tacho_i2c_data_t *i2c_slave_get_data(void)
{
  return &i2c_data;
}

/* -------------------------------------------------------------------------*
 *  HAL I2C Slave Callbacks                                                  *
 * -------------------------------------------------------------------------*/

/**
 * @brief  Address matched callback.
 *         Direction == TRANSMITTER means master is reading (slave transmits).
 *         Direction == RECEIVER    means master is writing (slave receives).
 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c,
                           uint8_t TransferDirection,
                           uint16_t AddrMatchCode)
{
  (void)AddrMatchCode;

  if (hi2c->Instance != i2c_handle->Instance)
  {
    return;
  }

  if (TransferDirection == I2C_DIRECTION_RECEIVE)
  {
    /* Master is writing to us: first byte will be the register address */
    is_write_transaction = 1u;
    write_byte_count = 0u;
    snapshot_done = 0u;
    /* Arm RX for the register-address byte */
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, &rx_byte, 1u,
                                  I2C_FIRST_FRAME);
  }
  else
  {
    /* Master is reading from us: transmit from snapshot */
    is_write_transaction = 0u;
    if (snapshot_done == 0u)
    {
      /* No preceding write set reg_ptr; snapshot now */
      memcpy(i2c_rd_buf, &i2c_data, sizeof(tacho_i2c_data_t));
      snapshot_done = 1u;
    }
    /* Clamp reg_ptr */
    if (reg_ptr >= (uint8_t)sizeof(tacho_i2c_data_t))
    {
      reg_ptr = 0u;
    }
    tx_byte = i2c_rd_buf[reg_ptr];
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &tx_byte, 1u,
                                   I2C_FIRST_AND_NEXT_FRAME);
  }
}

/**
 * @brief  Slave RX complete callback -- one byte received from master.
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance != i2c_handle->Instance)
  {
    return;
  }

  if (snapshot_done == 0u)
  {
    /* First byte received = register address; clamp to valid range */
    reg_ptr = (rx_byte < (uint8_t)sizeof(tacho_i2c_data_t)) ?
              rx_byte : 0u;
    memcpy(i2c_rd_buf, &i2c_data, sizeof(tacho_i2c_data_t));
    snapshot_done = 1u;
    write_byte_count = 0u;
  }
  else
  {
    /* Subsequent bytes = data to write */
    uint8_t target_offset = (uint8_t)(
        (reg_ptr + write_byte_count) % (uint8_t)sizeof(tacho_i2c_data_t));

    i2c_wr_buf[target_offset] = rx_byte;
    write_byte_count++;
  }

  /* Keep receiving subsequent bytes */
  HAL_I2C_Slave_Seq_Receive_IT(hi2c, &rx_byte, 1u,
                                I2C_NEXT_FRAME);
}

/**
 * @brief  Slave TX complete callback -- one byte sent to master.
 *         Increment reg_ptr and queue the next byte.
 */
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance != i2c_handle->Instance)
  {
    return;
  }

  reg_ptr = (uint8_t)((reg_ptr + 1u) % (uint8_t)sizeof(tacho_i2c_data_t));
  tx_byte = i2c_rd_buf[reg_ptr];
  HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &tx_byte, 1u,
                                 I2C_NEXT_FRAME);
}

/**
 * @brief  Listen complete callback -- STOP condition detected.
 *         Commit tacho_timeout if fully written, then restart listening.
 */
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance != i2c_handle->Instance)
  {
    return;
  }

  if (is_write_transaction != 0u)
  {
    /*
     * Commit tacho_timeout (offsets 0x02-0x03) if both bytes were staged.
     * The write spans [reg_ptr .. reg_ptr + write_byte_count - 1] mod struct.
     * Handle wrap-around: if first_written > last_written the range wraps.
     */
    if (write_byte_count > 0u)
    {
      uint8_t struct_size   = (uint8_t)sizeof(tacho_i2c_data_t);
      uint8_t first_written = reg_ptr;
      uint8_t last_written  = (uint8_t)(
          (reg_ptr + write_byte_count - 1u) % struct_size);

      uint8_t covers_02, covers_03;
      if (first_written <= last_written)
      {
        /* No wrap-around */
        covers_02 = (first_written <= TACHO_TIMEOUT_OFFSET_FIRST) &&
                    (TACHO_TIMEOUT_OFFSET_FIRST <= last_written);
        covers_03 = (first_written <= TACHO_TIMEOUT_OFFSET_LAST) &&
                    (TACHO_TIMEOUT_OFFSET_LAST  <= last_written);
      }
      else
      {
        /* Wrap-around: range covers [first_written..struct_size-1] + [0..last_written] */
        covers_02 = (TACHO_TIMEOUT_OFFSET_FIRST >= first_written) ||
                    (TACHO_TIMEOUT_OFFSET_FIRST <= last_written);
        covers_03 = (TACHO_TIMEOUT_OFFSET_LAST  >= first_written) ||
                    (TACHO_TIMEOUT_OFFSET_LAST  <= last_written);
      }

      if (covers_02 && covers_03)
      {
        uint16_t new_val;
        memcpy(&new_val,
               &i2c_wr_buf[TACHO_TIMEOUT_OFFSET_FIRST],
               sizeof(uint16_t));
        i2c_data.tacho_timeout = new_val;
      }
    }
  }

  /* Reset state and restart listening */
  is_write_transaction = 0u;
  write_byte_count     = 0u;
  snapshot_done        = 0u;

  HAL_I2C_EnableListen_IT(hi2c);
}

/**
 * @brief  Error callback -- NACK or bus error.
 *         Restart listening.
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance != i2c_handle->Instance)
  {
    return;
  }

  /* Ignore NACK on last byte of a read (expected) */
  uint32_t err = HAL_I2C_GetError(hi2c);
  if (err == HAL_I2C_ERROR_AF)
  {
    /* Normal NACK at end of master read -- restart listen */
    HAL_I2C_EnableListen_IT(hi2c);
    return;
  }

  /* For other errors, reset state and restart */
  is_write_transaction = 0u;
  write_byte_count     = 0u;
  snapshot_done        = 0u;

  HAL_I2C_EnableListen_IT(hi2c);
}
