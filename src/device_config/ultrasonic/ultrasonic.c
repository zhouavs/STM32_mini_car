#include "ultrasonic.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdlib.h>

static Device_ultrasonic devices[DEVICE_ULTRASONIC_COUNT] = {
  [DEVICE_ULTRASONIC_1] = {
    .name = DEVICE_ULTRASONIC_1,
  },
};
// 关联 trig 设备
static const Device_GPIO_name relate_trig[DEVICE_ULTRASONIC_COUNT] = {
  [DEVICE_ULTRASONIC_1] = DEVICE_ULTRASONIC_TRIG,
};
// 关联 echo 设备
static const Device_GPIO_name relate_echo[DEVICE_ULTRASONIC_COUNT] = {
  [DEVICE_ULTRASONIC_1] = DEVICE_ULTRASONIC_ECHO,
};
// 关联计时器
static const Device_timer_name relate_timer[DEVICE_ULTRASONIC_COUNT] = {
  [DEVICE_ULTRASONIC_1] = DEVICE_TIMER_TIM11,
};

errno_t Device_config_ultrasonic_register(void) {
  errno_t err = Device_ultrasonic_module_init();
  if (err) return err;
  
  for (Device_ultrasonic_name name = 0; name < DEVICE_ULTRASONIC_COUNT; ++name) {
    err = Device_GPIO_find(&devices[name].trig, relate_trig[name]);
    if (err) return err;

    err = Device_GPIO_find(&devices[name].echo, relate_echo[name]);
    if (err) return err;

    err = Device_timer_find(&devices[name].timer, relate_timer[name]);
    if (err) return err;

    err = Device_ultrasonic_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
