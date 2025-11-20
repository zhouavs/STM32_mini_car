#include "i2c.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "device/i2c/i2c.h"

static errno_t receive(const Device_I2C *const pd, uint8_t *data, uint16_t len);
static errno_t transmit(const Device_I2C *const pd, uint8_t *const data, uint16_t len);
static errno_t master_receive_IT(const Device_I2C *const pd, uint8_t *data, uint16_t len);
static errno_t master_transmit_IT(const Device_I2C *const pd, uint8_t *const data, uint16_t len);
static errno_t master_receive_DMA(const Device_SPI *const pd, uint8_t *data, uint16_t len);
static errno_t master_transmit_DMA(const Device_SPI *const pd, uint8_t *const data, uint16_t len);

static const Driver_SPI_ops ops = {
  .master_receive = receive,
  .master_transmit = transmit,
  .master_receive_IT = master_receive_IT,
  .master_transmit_IT = master_transmit_IT,
  .master_receive_DMA = master_receive_DMA,
  .master_transmit_DMA = master_transmit_DMA,
};

errno_t Driver_I2C_get_ops(const Driver_I2C_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}

static errno_t receive(const Device_I2C *const pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)pd->channel, pd->slave_addr, data, len, len * 10);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t transmit(const Device_I2C *const pd, uint8_t *const data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)pd->channel, pd->slave_addr, data, len, len * 10);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t master_receive_IT(const Device_I2C *const pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Receive_IT((I2C_HandleTypeDef *)pd->channel, pd->slave_addr, data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t master_transmit_IT(const Device_I2C *const pd, uint8_t *const data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_IT((I2C_HandleTypeDef *)pd->channel, pd->slave_addr, data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t master_receive_DMA(const Device_I2C *const pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Receive_DMA((I2C_HandleTypeDef *)pd->channel, pd->slave_addr, data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t master_transmit_DMA(const Device_I2C *const pd, uint8_t *const data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA((I2C_HandleTypeDef *)pd->channel, pd->slave_addr, data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}
