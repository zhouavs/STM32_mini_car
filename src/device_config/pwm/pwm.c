#include "pwm.h"
#include "stm32f4xx_hal.h"
#include "driver/pwm/pwm.h"
#include "Core/Inc/tim.h"

static Device_PWM devices[DEVICE_PWM_COUNT] = {
  [DEVICE_PWM_TIM_1_CH_1] = {
    .name = DEVICE_PWM_TIM_1_CH_1,
    .instance = &htim1,
    .channel = TIM_CHANNEL_1,
  },
  [DEVICE_PWM_TIM_3_CH_3] = {
    .name = DEVICE_PWM_TIM_3_CH_3,
    .instance = &htim3,
    .channel = TIM_CHANNEL_3,
  },
  [DEVICE_PWM_TIM_8_CH_1] = {
    .name = DEVICE_PWM_TIM_8_CH_1,
    .instance = &htim8,
    .channel = TIM_CHANNEL_1,
  },
  [DEVICE_PWM_TIM_8_CH_2] = {
    .name = DEVICE_PWM_TIM_8_CH_2,
    .instance = &htim8,
    .channel = TIM_CHANNEL_2,
  },
  [DEVICE_PWM_TIM_8_CH_3] = {
    .name = DEVICE_PWM_TIM_8_CH_3,
    .instance = &htim8,
    .channel = TIM_CHANNEL_3,
  },
  [DEVICE_PWM_TIM_8_CH_4] = {
    .name = DEVICE_PWM_TIM_8_CH_4,
    .instance = &htim8,
    .channel = TIM_CHANNEL_4,
  },
};

errno_t Device_config_PWM_register(void) {
  errno_t err = Device_PWM_module_init();
  if (err) return err;
  
  for (Device_PWM_name name = 0; name < DEVICE_PWM_COUNT; ++name) {
    err = Device_PWM_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
