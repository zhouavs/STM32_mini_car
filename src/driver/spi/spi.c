#include "spi.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "device/spi/spi.h"

static errno_t receive(Device_SPI *pd, uint8_t *data, uint16_t len);
static errno_t transmit(Device_SPI *pd, uint8_t *data, uint16_t len);
static errno_t receive_DMA(Device_SPI *pd, uint8_t *data, uint16_t len);
static errno_t transmit_DMA(Device_SPI *pd, uint8_t *data, uint16_t len);

static const Driver_SPI_ops ops = {
  .receive = receive,
  .transmit = transmit,
  .receive_DMA = receive_DMA,
  .transmit_DMA = transmit_DMA,
};

static errno_t receive(Device_SPI *pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_SPI_Receive((SPI_HandleTypeDef *)pd->channel, data, len, len * 10);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t transmit(Device_SPI *pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_SPI_Transmit((SPI_HandleTypeDef *)pd->channel, data, len, len * 10);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t receive_DMA(Device_SPI *pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_SPI_Receive_DMA((SPI_HandleTypeDef *)pd->channel, data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t transmit_DMA(Device_SPI *pd, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA((SPI_HandleTypeDef *)pd->channel, data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

errno_t Driver_SPI_get_ops(const Driver_SPI_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}
