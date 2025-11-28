#include "timer.h"
#include "stm32f4xx_hal.h"
#include "driver/timer/timer.h"
#include "Core/Inc/tim.h"

static Device_timer devices[DEVICE_TIMER_COUNT] = {
  [DEVICE_TIMER_SYSTICK] = {
    .name = DEVICE_TIMER_SYSTICK,
    .type = DEVICE_TIMER_TYPE_SYSTICK,
    .instance = SysTick,
  },
  [DEVICE_TIMER_TIM2] = {
    .name = DEVICE_TIMER_TIM2,
    .type = DEVICE_TIMER_TYPE_GENERAL,
    .instance = &htim2,
  },
  [DEVICE_TIMER_TIM6] = {
    .name = DEVICE_TIMER_TIM6,
    .type = DEVICE_TIMER_TYPE_GENERAL,
    .instance = &htim6,
  },
  [DEVICE_TIMER_TIM7] = {
    .name = DEVICE_TIMER_TIM7,
    .type = DEVICE_TIMER_TYPE_GENERAL,
    .instance = &htim7,
  },
};

errno_t Device_config_timer_register_all_device(void) {
  errno_t err = Device_timer_module_init();
  if (err) return err;
  
  for (Device_timer_name name = 0; name < DEVICE_TIMER_COUNT; ++name) {
    err = Device_timer_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim == &htim2) {
    Device_timer_PeriodElapsedCallback(&devices[DEVICE_TIMER_TIM2]);
  } else if (htim == &htim6) {
    Device_timer_PeriodElapsedCallback(&devices[DEVICE_TIMER_TIM6]);
  } else if (htim == &htim7) {
    Device_timer_PeriodElapsedCallback(&devices[DEVICE_TIMER_TIM7]);
  }
}
