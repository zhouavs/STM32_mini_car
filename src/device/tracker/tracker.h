#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdint.h>

#define DEVICE_TRACKER_IN_COUNT 7

typedef enum {
  DEVICE_TRACKER_1,
  DEVICE_TRACKER_COUNT,
} Device_tracker_name;

struct Device_tracker;
struct Device_tracker_ops;

typedef struct Device_tracker {
  const Device_tracker_name name;
  Device_GPIO *ins[DEVICE_TRACKER_IN_COUNT];
  const struct Device_tracker_ops *ops;
} Device_tracker;

typedef struct Device_tracker_ops {
  errno_t (*init)(Device_tracker *const pd);
  errno_t (*get_line_center)(Device_tracker *const pd, uint8_t *rt_direction_ptr);
} Device_tracker_ops;

// 全局方法
errno_t Device_tracker_module_init(void);
errno_t Device_tracker_register(Device_tracker *const pd);
errno_t Device_tracker_find(Device_tracker **pd_ptr, const Device_tracker_name name);
