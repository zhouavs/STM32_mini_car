#pragma once

#include "common/errno/errno.h"
#include "device/pwm/pwm.h"
#include <stdint.h>
#include <stdbool.h>

typedef int8_t angle_t;

typedef enum {
  DEVICE_SERVO_1,
  DEVICE_SERVO_COUNT,
} Device_servo_name;

struct Device_servo;
struct Device_servo_ops;

typedef struct Device_servo {
  const Device_servo_name name;
  bool running;
  angle_t angle;
  const Device_PWM *pwm;
  const struct Device_servo_ops *ops;
} Device_servo;

typedef struct Device_servo_ops {
  errno_t (*init)(Device_servo *const pd);
  errno_t (*set_angle)(Device_servo *const pd, const angle_t angle);
  errno_t (*start)(Device_servo *const pd);
  errno_t (*stop)(Device_servo *const pd);
} Device_servo_ops;

// 全局方法
errno_t Device_servo_module_init(void);
errno_t Device_servo_register(Device_servo *const pd);
errno_t Device_servo_find(Device_servo **pd_ptr, const Device_servo_name name);
