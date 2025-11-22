#include "timer.h"
#include "stm32f4xx_hal.h"
#include "device/timer/timer.h"
#include "driver/timer/timer.h"
#include "Core/Inc/tim.h"

static Device_timer devices[DEVICE_TIMER_COUNT] = {
  [DEVICE_TIMER_SYSTICK] = {
    .name = DEVICE_TIMER_SYSTICK,
    .type = DEVICE_TIMER_TYPE_SYSTICK,
    .channel = SysTick,
  },
  [DEVICE_TIMER_TIM2] = {
    .name = DEVICE_TIMER_TIM2,
    .type = DEVICE_TIMER_TYPE_GENERAL,
    .channel = &htim2,
  },
};

errno_t Device_config_timer_register_all_device(void) {
  for (Device_timer_name name = 0; name < DEVICE_TIMER_COUNT; ++name) {
    errno_t err = Device_timer_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim == &htim2) {
    Device_timer_PeriodElapsedCallback(&devices[DEVICE_TIMER_TIM2]);
  }
}
