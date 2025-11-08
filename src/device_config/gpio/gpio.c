#include "device/gpio/gpio.h"
#include "driver/gpio/gpio.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "device/keyboard/keyboard.h"

static Device_GPIO devices[] = {
  [DEVICE_GPIO_NO_NAME]={
    0
  },
  [DEVICE_LED_1]={
    .name = DEVICE_LED_1,
    .port = GPIOB,
    .pin = GPIO_PIN_12,
  },
  [DEVICE_LED_2]={
    .name = DEVICE_LED_2,
    .port = GPIOB,
    .pin = GPIO_PIN_13,
  },
  [DEVICE_LED_3]={
    .name = DEVICE_LED_3,
    .port = GPIOB,
    .pin = GPIO_PIN_14,
  },
  [DEVICE_LED_4]={
    .name = DEVICE_LED_4,
    .port = GPIOB,
    .pin = GPIO_PIN_15,
  },
  [DEVICE_KEY_1]={
    .name = DEVICE_KEY_1,
    .port = GPIOA,
    .pin = GPIO_PIN_0,
  },
  [DEVICE_KEY_2]={
    .name = DEVICE_KEY_2,
    .port = GPIOA,
    .pin = GPIO_PIN_1,
  },
  [DEVICE_KEY_3]={
    .name = DEVICE_KEY_3,
    .port = GPIOA,
    .pin = GPIO_PIN_2,
  },
  [DEVICE_KEY_4]={
    .name = DEVICE_KEY_4,
    .port = GPIOA,
    .pin = GPIO_PIN_3,
  },
};

errno_t Device_config_GPIO_register_all_device(void) {
  size_t device_cnt = sizeof(devices) / sizeof(Device_GPIO);
  for (size_t i = 0; i < device_cnt; ++i) {
    if (!devices[i].name) continue;
    errno_t err = Device_GPIO_register(&devices[i]);
    if (err) return err;
  }

  return ESUCCESS;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  switch (GPIO_Pin) {
    case GPIO_PIN_0: {
      Device_GPIO_EXTI_callback(&devices[DEVICE_KEY_1]);
      Device_keyboard_EXTI_callback(DEVICE_KEY_1);
      break;
    }
    case GPIO_PIN_1: {
      Device_GPIO_EXTI_callback(&devices[DEVICE_KEY_2]);
      Device_keyboard_EXTI_callback(DEVICE_KEY_2);
      break;
    }
    case GPIO_PIN_2: {
      Device_GPIO_EXTI_callback(&devices[DEVICE_KEY_3]);
      Device_keyboard_EXTI_callback(DEVICE_KEY_3);
      break;
    }
    case GPIO_PIN_3: {
      Device_GPIO_EXTI_callback(&devices[DEVICE_KEY_4]);
      Device_keyboard_EXTI_callback(DEVICE_KEY_4);
      break;
    }
    default: {
      break;
    }
  }
}
