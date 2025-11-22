#include "timer.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "Core/Inc/tim.h"
#include "device/timer/timer.h"

static errno_t is_running(const Device_timer *const pd, bool *rt_running_ptr);
static errno_t start(const Device_timer *const pd);
static errno_t stop(const Device_timer *const pd);
static errno_t get_count(const Device_timer *const pd, uint32_t *rt_count_ptr);
static errno_t set_prescaler(const Device_timer *const pd, uint16_t value);
static errno_t set_clock_division(const Device_timer *const pd, uint8_t value);
static errno_t set_auto_reload_register(const Device_timer *const pd, uint32_t value);
static errno_t get_source_frequent(const Device_timer *const pd, uint32_t *rt_frequent_ptr);

static const Driver_timer_ops ops = {
  .is_running = is_running,
  .start = start,
  .stop = stop,
  .get_count = get_count,
  .set_prescaler = set_prescaler,
  .set_clock_division = set_clock_division,
  .set_auto_reload_register = set_auto_reload_register,
  .get_source_frequent = get_source_frequent,
};

static errno_t is_running(const Device_timer *const pd, bool *rt_running_ptr) {
  if (pd == NULL) return EINVAL;

  switch (pd->type) {
    case DEVICE_TIMER_TYPE_SYSTICK:
      *rt_running_ptr = (((SysTick_Type *)pd->channel)->CTRL & SysTick_CTRL_ENABLE_Msk) != 0;
      return ESUCCESS;
    case DEVICE_TIMER_TYPE_GENERAL:
      *rt_running_ptr = (((TIM_HandleTypeDef *)pd->channel)->Instance->CR1 & TIM_CR1_CEN) != 0;
      return ESUCCESS;
  }

  return EINVAL;
}

static errno_t start(const Device_timer *const pd) {
  if (pd == NULL) return EINVAL;

  switch (pd->type) {
    case DEVICE_TIMER_TYPE_SYSTICK:
      HAL_ResumeTick();
      return ESUCCESS;
    case DEVICE_TIMER_TYPE_GENERAL:
      HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *)pd->channel);
      return ESUCCESS;
  }

  return EINVAL;
}

static errno_t stop(const Device_timer *const pd) {
  if (pd == NULL) return EINVAL;

  switch (pd->type) {
    case DEVICE_TIMER_TYPE_SYSTICK:
      HAL_SuspendTick();
      return ESUCCESS;
    case DEVICE_TIMER_TYPE_GENERAL:
      HAL_TIM_Base_Stop_IT((TIM_HandleTypeDef *)pd->channel);
      return ESUCCESS;
  }
  
  return EINVAL;
}

static errno_t get_count(const Device_timer *const pd, uint32_t *rt_count_ptr) {
  if (pd == NULL) return EINVAL;

  if (pd->type == DEVICE_TIMER_TYPE_SYSTICK) {
    *rt_count_ptr = HAL_GetTick();
    return ESUCCESS;
  }

  return EINVAL;
}

static errno_t set_prescaler(const Device_timer *const pd, uint16_t value) {
  if (pd == NULL || pd->type != DEVICE_TIMER_TYPE_GENERAL) return EINVAL;

  __HAL_TIM_SET_PRESCALER((TIM_HandleTypeDef *)pd->channel, value);

  return ESUCCESS;
}

static errno_t set_clock_division(const Device_timer *const pd, uint8_t value) {
  if (pd == NULL || pd->type != DEVICE_TIMER_TYPE_GENERAL) return EINVAL;

  switch (value) {
    case 1:
      __HAL_TIM_SET_CLOCKDIVISION((TIM_HandleTypeDef *)pd->channel, TIM_CLOCKDIVISION_DIV1);
      return ESUCCESS;
    case 2:
      __HAL_TIM_SET_CLOCKDIVISION((TIM_HandleTypeDef *)pd->channel, TIM_CLOCKDIVISION_DIV2);
      return ESUCCESS;
    case 4:
      __HAL_TIM_SET_CLOCKDIVISION((TIM_HandleTypeDef *)pd->channel, TIM_CLOCKDIVISION_DIV4);
      return ESUCCESS;
  }

  return EINVAL;
}

static errno_t set_auto_reload_register(const Device_timer *const pd, uint32_t value) {
  if (pd == NULL || pd->type != DEVICE_TIMER_TYPE_GENERAL) return EINVAL;

  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->channel;

  if (htim == &htim2) {
    // tim2 的 AutoReload Register 寄存器有 32 位, 可以完整保存 value
    __HAL_TIM_SET_AUTORELOAD(htim, value);
    return ESUCCESS;
  }

  return EINVAL;
}

static errno_t get_source_frequent(const Device_timer *const pd, uint32_t *rt_frequent_ptr) {
  if (pd == NULL || pd->type != DEVICE_TIMER_TYPE_GENERAL) return EINVAL;

  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->channel;

  if (htim == &htim2) {
    *rt_frequent_ptr = HAL_RCC_GetPCLK1Freq() * 2;
    return ESUCCESS;
  }

  return EINVAL;
}

errno_t Driver_timer_get_ops(const Driver_timer_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}
