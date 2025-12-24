#include "usart.h"
#include "stm32f4xx_hal.h"
#include "driver/usart/usart.h"
#include "Core/Inc/usart.h"

static Device_USART devices[DEVICE_USART_COUNT] = {
  [DEVICE_USART_DEBUG] = {
    .name = DEVICE_USART_DEBUG,
    .buffer_size = 255,
    .instance = &huart1,
  },
  [DEVICE_USART_WIFI_BLUETOOTH] = {
    .name = DEVICE_USART_WIFI_BLUETOOTH,
    .buffer_size = 255,
    .instance = &huart3,
  },
};

errno_t Device_config_USART_register(void) {
  errno_t err = Device_USART_module_init();
  if (err) return err;
  
  for (Device_USART_name name = 0; name < DEVICE_USART_COUNT; ++name) {
    err = Device_USART_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart1) {
    Device_USART_TxCpltCallback(&devices[DEVICE_USART_DEBUG]);
  } else if (huart == &huart3) {
    Device_USART_TxCpltCallback(&devices[DEVICE_USART_WIFI_BLUETOOTH]);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart1) {
    Device_USART_RxCpltCallback(&devices[DEVICE_USART_DEBUG]);
  } else if (huart == &huart3) {
    Device_USART_RxCpltCallback(&devices[DEVICE_USART_WIFI_BLUETOOTH]);
  }
}
