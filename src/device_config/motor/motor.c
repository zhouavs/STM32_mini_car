#include "motor.h"
#include "device/gpio/gpio.h"
#include "device/pwm/pwm.h"
#include <stdlib.h>

static Device_motor devices[DEVICE_MOTOR_COUNT] = {
  [DEVICE_MOTOR_HEAD_LEFT] = {
    .name = DEVICE_MOTOR_HEAD_LEFT,
    .status = DEVICE_MOTOR_STATUS_STOP,
    .speed = 0,
  },
  [DEVICE_MOTOR_HEAD_RIGHT] = {
    .name = DEVICE_MOTOR_HEAD_RIGHT,
    .status = DEVICE_MOTOR_STATUS_STOP,
    .speed = 0,
  },
  [DEVICE_MOTOR_TAIL_LEFT] = {
    .name = DEVICE_MOTOR_TAIL_LEFT,
    .status = DEVICE_MOTOR_STATUS_STOP,
    .speed = 0,
  },
  [DEVICE_MOTOR_TAIL_RIGHT] = {
    .name = DEVICE_MOTOR_TAIL_RIGHT,
    .status = DEVICE_MOTOR_STATUS_STOP,
    .speed = 0,
  },
};
// 关联的 in_1 in_2 pwm 设备
static const Device_GPIO_name relate_in_1[DEVICE_MOTOR_COUNT] = {
  [DEVICE_MOTOR_HEAD_LEFT] = DEVICE_MOTOR_HEAD_LEFT_IN_1,
  [DEVICE_MOTOR_HEAD_RIGHT] = DEVICE_MOTOR_HEAD_RIGHT_IN_1,
  [DEVICE_MOTOR_TAIL_LEFT] = DEVICE_MOTOR_TAIL_LEFT_IN_1,
  [DEVICE_MOTOR_TAIL_RIGHT] = DEVICE_MOTOR_TAIL_RIGHT_IN_1,
};
static const Device_GPIO_name relate_in_2[DEVICE_MOTOR_COUNT] = {
  [DEVICE_MOTOR_HEAD_LEFT] = DEVICE_MOTOR_HEAD_LEFT_IN_2,
  [DEVICE_MOTOR_HEAD_RIGHT] = DEVICE_MOTOR_HEAD_RIGHT_IN_2,
  [DEVICE_MOTOR_TAIL_LEFT] = DEVICE_MOTOR_TAIL_LEFT_IN_2,
  [DEVICE_MOTOR_TAIL_RIGHT] = DEVICE_MOTOR_TAIL_RIGHT_IN_2,
};
static const Device_PWM_name relate_pwm[DEVICE_MOTOR_COUNT] = {
  [DEVICE_MOTOR_HEAD_LEFT] = DEVICE_PWM_TIM_8_CH_1,
  [DEVICE_MOTOR_HEAD_RIGHT] = DEVICE_PWM_TIM_8_CH_2,
  [DEVICE_MOTOR_TAIL_LEFT] = DEVICE_PWM_TIM_8_CH_3,
  [DEVICE_MOTOR_TAIL_RIGHT] = DEVICE_PWM_TIM_8_CH_4,
};

errno_t Device_config_motor_register(void) {
  errno_t err = Device_motor_module_init();
  if (err) return err;

  for (Device_motor_name name = 0; name < DEVICE_MOTOR_COUNT; ++name) {
    err = Device_GPIO_find(&devices[name].in_1, relate_in_1[name]);
    if (err) return err;
    err = Device_GPIO_find(&devices[name].in_2, relate_in_2[name]);
    if (err) return err;
    err = Device_PWM_find(&devices[name].pwm, relate_pwm[name]);

    err = Device_motor_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
