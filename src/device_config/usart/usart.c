#include "usart.h"
#include "stm32f4xx_hal.h"
#include "device/usart/usart.h"
#include "driver/usart/usart.h"
#include "Core/Inc/usart.h"

static Device_USART devices[DEVICE_USART_COUNT] = {
  [DEVICE_USART_DEBUG] = {
    .name = DEVICE_USART_DEBUG,
    .channel = &huart1,
  },
};

errno_t Device_config_USART_register_all_device(void) {
  for (Device_USART_name name = 0; name < DEVICE_USART_COUNT; ++name) {
    errno_t err = Device_USART_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart1) {
    Device_USART_TxCpltCallback(&devices[DEVICE_USART_DEBUG]);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart1) {
    Device_USART_RxCpltCallback(&devices[DEVICE_USART_DEBUG]);
  }
}
