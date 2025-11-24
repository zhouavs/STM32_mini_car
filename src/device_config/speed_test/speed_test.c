#include "speed_test.h"
#include "device/gpio/gpio.h"
#include "device/pwm/pwm.h"
#include <stdlib.h>

static void in_head_left_callback(void);
static void in_head_right_callback(void);
static void in_tail_left_callback(void);
static void in_tail_right_callback(void);

static Device_speed_test devices[DEVICE_SPEED_TEST_COUNT] = {
  [DEVICE_SPEED_TEST_HEAD_LEFT] = {
    .name = DEVICE_SPEED_TEST_HEAD_LEFT,
  },
  [DEVICE_SPEED_TEST_HEAD_RIGHT] = {
    .name = DEVICE_SPEED_TEST_HEAD_RIGHT,
  },
  [DEVICE_SPEED_TEST_TAIL_LEFT] = {
    .name = DEVICE_SPEED_TEST_TAIL_LEFT,
  },
  [DEVICE_SPEED_TEST_TAIL_RIGHT] = {
    .name = DEVICE_SPEED_TEST_TAIL_RIGHT,
  },
};

// 关联的GPIO信号输入设备
static const Device_GPIO_name relate_in[DEVICE_SPEED_TEST_COUNT] = {
  [DEVICE_SPEED_TEST_HEAD_LEFT] = DEVICE_SPEED_TEST_HEAD_LEFT_IN,
  [DEVICE_SPEED_TEST_HEAD_RIGHT] = DEVICE_SPEED_TEST_HEAD_RIGHT_IN,
  [DEVICE_SPEED_TEST_TAIL_LEFT] = DEVICE_SPEED_TEST_TAIL_LEFT_IN,
  [DEVICE_SPEED_TEST_TAIL_RIGHT] = DEVICE_SPEED_TEST_TAIL_RIGHT_IN,
};
// 关联定时器
static const Device_timer_name relate_timer[DEVICE_SPEED_TEST_COUNT] = {
  [DEVICE_SPEED_TEST_HEAD_LEFT] = DEVICE_TIMER_SYSTICK,
  [DEVICE_SPEED_TEST_HEAD_RIGHT] = DEVICE_TIMER_SYSTICK,
  [DEVICE_SPEED_TEST_TAIL_LEFT] = DEVICE_TIMER_SYSTICK,
  [DEVICE_SPEED_TEST_TAIL_RIGHT] = DEVICE_TIMER_SYSTICK,
};
// 关联的回调函数
static void (*const callbacks[DEVICE_SPEED_TEST_COUNT])(void) = {
  [DEVICE_SPEED_TEST_HEAD_LEFT] = in_head_left_callback,
  [DEVICE_SPEED_TEST_HEAD_RIGHT] = in_head_right_callback,
  [DEVICE_SPEED_TEST_TAIL_LEFT] = in_tail_left_callback,
  [DEVICE_SPEED_TEST_TAIL_RIGHT] = in_tail_right_callback,
};

errno_t Device_config_speed_test_register_all_device(void) {
  errno_t err = Device_speed_test_module_init();
  if (err) return err;
  
  for (Device_speed_test_name name = 0; name < DEVICE_SPEED_TEST_COUNT; ++name) {
    err = Device_GPIO_find(&devices[name].in, relate_in[name]);
    if (err) return err;
    
    // 不知道为什么, 即使设置为了单边沿(并且通过 HAL_EXTI_GetConfigLine 方式查看过配置确实生效), 在检测灯亮灭一次的情况下还是会产生两个中断, 目前暂未找到原因, 这里就暂且按照双边沿的配置计算转速了
    err = devices[name].in->ops->set_EXTI_handle(devices[name].in, DEVICE_GPIO_EXTI_TRIGGER_RISING_FALLING, callbacks[name]);
    if (err) return err;

    err = Device_timer_find(&devices[name].timer, relate_timer[name]);
    if (err) return err;

    err = Device_speed_test_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

static void in_head_left_callback(void) {
  Device_speed_test *pd = NULL;
  errno_t err = Device_speed_test_find(&pd, DEVICE_SPEED_TEST_HEAD_LEFT);
  if (err) return;

  Device_speed_test_EXTI_callback(pd);
}

static void in_head_right_callback(void) {
  Device_speed_test *pd = NULL;
  errno_t err = Device_speed_test_find(&pd, DEVICE_SPEED_TEST_HEAD_RIGHT);
  if (err) return;

  Device_speed_test_EXTI_callback(pd);
}

static void in_tail_left_callback(void) {
  Device_speed_test *pd = NULL;
  errno_t err = Device_speed_test_find(&pd, DEVICE_SPEED_TEST_TAIL_LEFT);
  if (err) return;

  Device_speed_test_EXTI_callback(pd);
}

static void in_tail_right_callback(void) {
  Device_speed_test *pd = NULL;
  errno_t err = Device_speed_test_find(&pd, DEVICE_SPEED_TEST_TAIL_RIGHT);
  if (err) return;

  Device_speed_test_EXTI_callback(pd);
}
