#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  DEVICE_ultrasonic_1,
  DEVICE_ultrasonic_COUNT,
} Device_ultrasonic_name;

struct Device_ultrasonic;
struct Device_ultrasonic_ops;

typedef struct Device_ultrasonic {
  const Device_ultrasonic_name name;
  Device_GPIO *trig;
  Device_GPIO *echo;
  Device_timer *timer;
  const struct Device_ultrasonic_ops *ops;
} Device_ultrasonic;

typedef struct Device_ultrasonic_ops {
  errno_t (*init)(Device_ultrasonic *const pd);
  errno_t (*read)(Device_ultrasonic *const pd, uint32_t *rt_data_ptr);
} Device_ultrasonic_ops;

// 全局方法
errno_t Device_ultrasonic_module_init(void);
errno_t Device_ultrasonic_register(Device_ultrasonic *const pd);
errno_t Device_ultrasonic_find(Device_ultrasonic **pd_ptr, const Device_ultrasonic_name name);
