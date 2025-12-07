#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"

typedef enum {
  DEVICE_KEY_1,
  DEVICE_KEY_2,
  DEVICE_KEY_3,
  DEVICE_KEY_4,
  DEVICE_KEY_COUNT
} Device_key_name;

typedef enum {
  DEVICE_KEYBOARD_1,
  DEVICE_KEYBOARD_COUNT,
} Device_keyboard_name;

struct Device_keyboard;
struct Device_keyboard_ops;

typedef struct Device_keyboard {
  const Device_keyboard_name name;
  const Device_key_name keys[DEVICE_KEY_COUNT]; // 关联的 KEY
  Device_GPIO *ins[DEVICE_KEY_COUNT]; // 关联的 GPIO 设备, 每个设备的下标需要和对应的 KEY 值相同
  const struct Device_keyboard_ops *ops;
} Device_keyboard;

typedef struct Device_keyboard_ops {
  errno_t (*read)(Device_key_name *key_name);
} Device_keyboard_ops;

errno_t Device_keyboard_module_init(void);
errno_t Device_keyboard_register(Device_keyboard *const pd);
errno_t Device_keyboard_find(const Device_keyboard **pd_ptr, const Device_keyboard_name);
errno_t Device_keyboard_in_EXTI_callback(const Device_keyboard *const pd, const Device_key_name key);
