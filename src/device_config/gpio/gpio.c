#include "device/gpio/gpio.h"
#include "driver/gpio/gpio.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

static Device_GPIO devices[] = {
  {
    .name = LED_1,
    .port = GPIOB,
    .pin = GPIO_PIN_12,
  },
  {
    .name = LED_2,
    .port = GPIOB,
    .pin = GPIO_PIN_13,
  },
  {
    .name = LED_3,
    .port = GPIOB,
    .pin = GPIO_PIN_14,
  },
  {
    .name = LED_4,
    .port = GPIOB,
    .pin = GPIO_PIN_15,
  },
  {
    .name = KEY_1,
    .port = GPIOA,
    .pin = GPIO_PIN_0,
  },
  {
    .name = KEY_2,
    .port = GPIOA,
    .pin = GPIO_PIN_1,
  },
  {
    .name = KEY_3,
    .port = GPIOA,
    .pin = GPIO_PIN_2,
  },
  {
    .name = KEY_4,
    .port = GPIOA,
    .pin = GPIO_PIN_3,
  },
};

errno_t Device_config_GPIO_register_all_device(void) {
  errno_t err = Device_GPIO_module_init();
  if (err) return err;

  size_t device_cnt = sizeof(devices) / sizeof(Device_GPIO);
  for (size_t i = 0; i < device_cnt; ++i) {
    // err = Driver_GPIO_init(&devices[i]);
    if (err) return err;
    err = Device_GPIO_register(&devices[i]);
    if (err) return err;
  }

  return ESUCCESS;
}
