#include "i2c.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "device/i2c/i2c.h"

static errno_t master_receive(const Device_I2C *const pd, uint8_t *data, uint16_t len);
static errno_t master_transmit(const Device_I2C *const pd, uint8_t *const data, uint16_t len);
static errno_t master_receive_IT(const Device_I2C *const pd, uint8_t *data, uint16_t len);
static errno_t master_transmit_IT(const Device_I2C *const pd, uint8_t *const data, uint16_t len);
static errno_t master_receive_DMA(const Device_I2C *const pd, uint8_t *data, uint16_t len);
static errno_t master_transmit_DMA(const Device_I2C *const pd, uint8_t *const data, uint16_t len);

static inline uint16_t get_param_addr(const Device_I2C *const pd);

static const Driver_I2C_ops ops = {
  .master_receive = master_receive,
  .master_transmit = master_transmit,
  .master_receive_IT = master_receive_IT,
  .master_transmit_IT = master_transmit_IT,
  .master_receive_DMA = master_receive_DMA,
  .master_transmit_DMA = master_transmit_DMA,
};

errno_t Driver_I2C_get_ops(const Driver_I2C_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}

static errno_t master_receive(const Device_I2C *const pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)pd->channel, get_param_addr(pd), data, len, len * 10);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t master_transmit(const Device_I2C *const pd, uint8_t *const data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)pd->channel, get_param_addr(pd), data, len, len * 10);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t master_receive_IT(const Device_I2C *const pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Receive_IT((I2C_HandleTypeDef *)pd->channel, get_param_addr(pd), data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t master_transmit_IT(const Device_I2C *const pd, uint8_t *const data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_IT((I2C_HandleTypeDef *)pd->channel, get_param_addr(pd), data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t master_receive_DMA(const Device_I2C *const pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Receive_DMA((I2C_HandleTypeDef *)pd->channel, get_param_addr(pd), data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t master_transmit_DMA(const Device_I2C *const pd, uint8_t *const data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA((I2C_HandleTypeDef *)pd->channel, get_param_addr(pd), data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

/**
 * @brief 获取 HAL 库发送的从机地址
 * 如果是 7 位地址则左移 1 位, 10 位地址不左移
 * @param pd 
 * @return uint16_t 
 */
static inline uint16_t get_param_addr(const Device_I2C *const pd) {
  const I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)pd->channel;
  if (hi2c->Init.AddressingMode == I2C_ADDRESSINGMODE_7BIT) {
    // 当前为 7 位地址模式
    return pd->slave_addr << 1;
  } else {
    // 当前为 10 位地址模式
    return pd->slave_addr;
  }
}
