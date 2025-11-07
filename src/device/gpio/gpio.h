#pragma once

#include "common/errno/errno.h"
#include <stdint.h>

typedef enum Device_GPIO_name {
  DEVICE_GPIO_NO_NAME,
  LED_1, LED_2, LED_3, LED_4,
  KEY_1, KEY_2, KEY_3, KEY_4,
} Device_GPIO_name;

typedef enum Device_GPIO_value {
  DEVICE_GPIO_PIN_RESET = 0,
  DEVICE_GPIO_PIN_SET = 1,
} Device_GPIO_value;

struct Device_GPIO;
struct Device_GPIO_ops;

typedef struct Device_GPIO {
  Device_GPIO_name name;
  void *port;
  uint16_t pin;
  const struct Device_GPIO_ops *ops;
} Device_GPIO;

typedef struct Device_GPIO_ops {
  errno_t (*init)(const Device_GPIO *const pd);
  errno_t (*read)(const Device_GPIO *const pd, Device_GPIO_value *value_ptr);
  errno_t (*write)(const Device_GPIO *const pd, const Device_GPIO_value value);
} Device_GPIO_ops;

typedef struct Driver_GPIO_ops {
  errno_t (*read)(const Device_GPIO *const pd, Device_GPIO_value *value_ptr);
  errno_t (*write)(const Device_GPIO *const pd, const Device_GPIO_value value);
} Driver_GPIO_ops;

errno_t Device_GPIO_module_init(void);
errno_t Device_GPIO_register(Device_GPIO *const pd);
errno_t Device_GPIO_find(Device_GPIO **pd_ptr, const Device_GPIO_name name);
