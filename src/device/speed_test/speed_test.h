#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdint.h>

typedef enum {
  DEVICE_SPEED_TEST_HEAD_LEFT,
  DEVICE_SPEED_TEST_HEAD_RIGHT,
  DEVICE_SPEED_TEST_TAIL_LEFT,
  DEVICE_SPEED_TEST_TAIL_RIGHT,
  DEVICE_SPEED_TEST_COUNT,
} Device_speed_test_name;

struct Device_speed_test;
struct Device_speed_test_ops;

typedef struct Device_speed_test {
  const Device_speed_test_name name;
  Device_GPIO *in;
  Device_timer *timer;
  const struct Device_speed_test_ops *ops;
} Device_speed_test;

typedef struct Device_speed_test_ops {
  errno_t (*init)(Device_speed_test *const pd);
  errno_t (*get_speed)(Device_speed_test *const pd, float *rt_speed_ptr);
} Device_speed_test_ops;

// 全局方法
errno_t Device_speed_test_module_init(void);
errno_t Device_speed_test_register(Device_speed_test *const pd);
errno_t Device_speed_test_find(Device_speed_test **pd_ptr, const Device_speed_test_name name);

// 中断回调
errno_t Device_speed_test_EXTI_callback(Device_speed_test *pd);