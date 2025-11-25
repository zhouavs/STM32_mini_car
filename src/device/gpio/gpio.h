#pragma once

#include "common/errno/errno.h"
#include <stdint.h>

typedef enum Device_GPIO_name {
  DEVICE_LED_1, DEVICE_LED_2, DEVICE_LED_3, DEVICE_LED_4,
  DEVICE_KEY_1, DEVICE_KEY_2, DEVICE_KEY_3, DEVICE_KEY_4,
  DEVICE_W25Q64_CS,
  DEVICE_ST7789V2_1_CS, DEVICE_ST7789V2_1_RST, DEVICE_ST7789V2_1_DC, DEVICE_ST7789V2_1_BACKLIGHT,
  DEVICE_MOTOR_HEAD_LEFT_IN_1, DEVICE_MOTOR_HEAD_LEFT_IN_2, DEVICE_MOTOR_HEAD_RIGHT_IN_1, DEVICE_MOTOR_HEAD_RIGHT_IN_2,
  DEVICE_MOTOR_TAIL_LEFT_IN_1, DEVICE_MOTOR_TAIL_LEFT_IN_2, DEVICE_MOTOR_TAIL_RIGHT_IN_1, DEVICE_MOTOR_TAIL_RIGHT_IN_2,
  DEVICE_SPEED_TEST_HEAD_LEFT_IN, DEVICE_SPEED_TEST_HEAD_RIGHT_IN, DEVICE_SPEED_TEST_TAIL_LEFT_IN, DEVICE_SPEED_TEST_TAIL_RIGHT_IN,
  DEVICE_TRACKER_IN_1, DEVICE_TRACKER_IN_2, DEVICE_TRACKER_IN_3, DEVICE_TRACKER_IN_4, DEVICE_TRACKER_IN_5, DEVICE_TRACKER_IN_6, DEVICE_TRACKER_IN_7,
  DEVICE_GPIO_COUNT,
} Device_GPIO_name;

typedef enum {
  DEVICE_GPIO_EXTI_TRIGGER_RISING,
  DEVICE_GPIO_EXTI_TRIGGER_FALLING,
  DEVICE_GPIO_EXTI_TRIGGER_RISING_FALLING,
} Device_GPIO_EXTI_trigger;

typedef enum Pin_value {
  PIN_VALUE_0 = 0,
  PIN_VALUE_1 = 1,
} Pin_value;

struct Device_GPIO;
struct Device_GPIO_ops;

typedef struct Device_GPIO {
  const Device_GPIO_name name;
  void *const port;
  const uint16_t pin;
  void *const exti_handle;
  const struct Device_GPIO_ops *ops;
} Device_GPIO;

typedef struct Device_GPIO_ops {
  errno_t (*init)(const Device_GPIO *const pd);
  errno_t (*read)(const Device_GPIO *const pd, Pin_value *value_ptr);
  errno_t (*write)(const Device_GPIO *const pd, const Pin_value value);
  errno_t (*set_EXTI_handle)(const Device_GPIO *const pd, Device_GPIO_EXTI_trigger trigger, void (*callback)(void));
} Device_GPIO_ops;

typedef struct Driver_GPIO_ops {
  errno_t (*read)(const Device_GPIO *const pd, Pin_value *value_ptr);
  errno_t (*write)(const Device_GPIO *const pd, const Pin_value value);
  errno_t (*set_EXTI_handle)(const Device_GPIO *const pd, Device_GPIO_EXTI_trigger trigger, void (*callback)(void));
} Driver_GPIO_ops;

errno_t Device_GPIO_module_init(void);
errno_t Device_GPIO_register(Device_GPIO *const pd);
errno_t Device_GPIO_find(Device_GPIO **pd_ptr, const Device_GPIO_name name);

errno_t Device_GPIO_EXTI_callback(Device_GPIO *pd);
