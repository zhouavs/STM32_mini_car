#include "irda.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdlib.h>

static void in_callback(void);

static Device_IRDA devices[DEVICE_IRDA_COUNT] = {
  [DEVICE_IRDA_1] = {
    .name = DEVICE_IRDA_1,
  },
};
// 关联 gpio 设备
static const Device_GPIO_name relate_in[DEVICE_IRDA_COUNT] = {
  [DEVICE_IRDA_1] = DEVICE_IRDA_IN,
};
// 关联计时器
static const Device_timer_name relate_timer[DEVICE_IRDA_COUNT] = {
  [DEVICE_IRDA_1] = DEVICE_TIMER_TIM7,
};
// 关联中断线回调函数
static void (*const relate_callbacks[DEVICE_IRDA_COUNT])(void) = {
  [DEVICE_IRDA_1] = in_callback,
};

errno_t Device_config_IRDA_register(void) {
  errno_t err = Device_IRDA_module_init();
  if (err) return err;
  
  for (Device_IRDA_name name = 0; name < DEVICE_IRDA_COUNT; ++name) {
    err = Device_GPIO_find(&devices[name].in, relate_in[name]);
    if (err) return err;

    err = devices[name].in->ops->set_EXTI_handle(devices[name].in, DEVICE_GPIO_EXTI_TRIGGER_RISING_FALLING, relate_callbacks[name]);
    if (err) return err;

    err = Device_timer_find(&devices[name].timer, relate_timer[name]);
    if (err) return err;

    err = Device_IRDA_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

static void in_callback(void) {
  Device_IRDA *pd = NULL;
  errno_t err = Device_IRDA_find(&pd, DEVICE_IRDA_1);
  if (err) return;

  Device_IRDA_in_EXTI_callback(pd);
}
