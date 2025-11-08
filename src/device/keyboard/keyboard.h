#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"

struct Device_keyboard;
struct Device_keyboard_ops;

typedef struct Device_keyboard {
  const struct Device_keyboard_ops *ops;
} Device_keyboard;

typedef struct Device_keyboard_ops {
  errno_t (*read)(Device_GPIO_name *key_name);
} Device_keyboard_ops;

errno_t Device_keyboard_module_init(void);
errno_t Device_keyboard_get_device(const Device_keyboard **pd_ptr);
errno_t Device_keyboard_EXTI_callback(const Device_GPIO *const pd);
