#include "usart.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "device/usart/usart.h"

static errno_t receive(Device_USART *pd, uint8_t *data, uint32_t len);
static errno_t transmit(Device_USART *pd, uint8_t *data, uint32_t len);
static errno_t receive_IT(Device_USART *pd, uint8_t *data, uint32_t len);
static errno_t transmit_IT(Device_USART *pd, uint8_t *data, uint32_t len);
static errno_t abort_receive(Device_USART *pd);
static errno_t abort_transmit(Device_USART *pd);

static const Driver_USART_ops ops = {
  .receive = receive,
  .transmit = transmit,
  .receive_IT = receive_IT,
  .transmit_IT = transmit_IT,
  .abort_receive = abort_receive,
  .abort_transmit = abort_transmit,
};

static errno_t receive(Device_USART *pd, uint8_t *data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_UART_Receive((UART_HandleTypeDef *)pd->channel, data, len, len * 10);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t transmit(Device_USART *pd, uint8_t *data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0 || len > 0xFFFF) return EINVAL;
  HAL_StatusTypeDef status = HAL_UART_Transmit((UART_HandleTypeDef *)pd->channel, data, len, len * 10);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t receive_IT(Device_USART *pd, uint8_t *data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  HAL_StatusTypeDef status = HAL_UART_Receive_IT((UART_HandleTypeDef *)pd->channel, data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t transmit_IT(Device_USART *pd, uint8_t *data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0 || len > 0xFFFF) return EINVAL;
  HAL_StatusTypeDef status = HAL_UART_Transmit_IT((UART_HandleTypeDef *)pd->channel, data, len);
  if (status == HAL_OK) return ESUCCESS;
  if (status == HAL_BUSY) return EBUSY;
  return EIO;
}

static errno_t abort_receive(Device_USART *pd) {
  HAL_StatusTypeDef status = HAL_UART_AbortReceive((UART_HandleTypeDef *)pd->channel);
  return status == HAL_OK ? ESUCCESS : EIO;
}

static errno_t abort_transmit(Device_USART *pd) {
  HAL_StatusTypeDef state = HAL_UART_AbortTransmit((UART_HandleTypeDef *)pd->channel);
  return state == HAL_OK ? ESUCCESS : EIO;
}

errno_t Driver_USART_get_ops(const Driver_USART_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}
