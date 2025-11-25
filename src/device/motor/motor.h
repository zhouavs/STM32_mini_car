#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device/pwm/pwm.h"
#include <stdint.h>

typedef uint8_t speed_t;

typedef enum {
  DEVICE_MOTOR_HEAD_LEFT,
  DEVICE_MOTOR_HEAD_RIGHT,
  DEVICE_MOTOR_TAIL_LEFT,
  DEVICE_MOTOR_TAIL_RIGHT,
  DEVICE_MOTOR_COUNT,
} Device_motor_name;

typedef enum {
  DEVICE_MOTOR_STATUS_STOP,
  DEVICE_MOTOR_STATUS_FORWARD,
  DEVICE_MOTOR_STATUS_BACKWARD,
} Device_motor_status;

struct Device_motor;
struct Device_motor_ops;

typedef struct Device_motor {
  const Device_motor_name name;
  Device_motor_status status;
  speed_t speed;
  Device_GPIO *in_1;
  Device_GPIO *in_2;
  const Device_PWM *pwm;
  const struct Device_motor_ops *ops;
} Device_motor;

typedef struct Device_motor_ops {
  errno_t (*init)(Device_motor *const pd);
  errno_t (*stop)(Device_motor *const pd);
  errno_t (*forward)(Device_motor *const pd, const speed_t speed);
  errno_t (*backward)(Device_motor *const pd, const speed_t speed);
} Device_motor_ops;

// 全局方法
errno_t Device_motor_module_init(void);
errno_t Device_motor_register(Device_motor *const pd);
errno_t Device_motor_find(Device_motor **pd_ptr, const Device_motor_name name);
