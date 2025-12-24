#include "servo.h"
#include "device/pwm/pwm.h"
#include <stdlib.h>

static Device_servo devices[DEVICE_SERVO_COUNT] = {
  [DEVICE_SERVO_1] = {
    .name = DEVICE_SERVO_1,
    .running = false,
    .angle = 0,
  },
};
// 关联 pwm 设备
static const Device_PWM_name relate_pwm[DEVICE_SERVO_COUNT] = {
  [DEVICE_SERVO_1] = DEVICE_PWM_TIM_3_CH_3,
};

errno_t Device_config_servo_register(void) {
  errno_t err = Device_servo_module_init();
  if (err) return err;
  
  for (Device_servo_name name = 0; name < DEVICE_SERVO_COUNT; ++name) {
    err = Device_PWM_find(&devices[name].pwm, relate_pwm[name]);

    err = Device_servo_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
