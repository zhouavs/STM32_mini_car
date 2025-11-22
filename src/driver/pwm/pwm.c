#include "pwm.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "Core/Inc/tim.h"
#include "device/pwm/pwm.h"

static errno_t is_running(const Device_PWM *const pd, bool *rt_running_ptr);
static errno_t start(const Device_PWM *const pd);
static errno_t stop(const Device_PWM *const pd);
static errno_t set_prescaler(const Device_PWM *const pd, uint16_t value);
static errno_t set_clock_division(const Device_PWM *const pd, uint8_t value);
static errno_t set_auto_reload_register(const Device_PWM *const pd, uint32_t value);
static errno_t set_compare(const Device_PWM *const pd, uint32_t value);
static errno_t get_source_frequent(const Device_PWM *const pd, uint32_t *rt_frequent_ptr);

static const Driver_pwm_ops ops = {
  .is_running = is_running,
  .start = start,
  .stop = stop,
  .set_prescaler = set_prescaler,
  .set_clock_division = set_clock_division,
  .set_auto_reload_register = set_auto_reload_register,
  .set_compare = set_compare,
  .get_source_frequent = get_source_frequent,
};

errno_t Driver_pwm_get_ops(const Driver_pwm_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}

static errno_t is_running(const Device_PWM *const pd, bool *rt_running_ptr) {
  if (pd == NULL || rt_running_ptr == NULL) return EINVAL;

  // 判断定时器是否在计数
  bool running = (((TIM_HandleTypeDef *)pd->instance)->Instance->CR1 & TIM_CR1_CEN) != 0;

  // 判断 pwm 通道输出使能
  switch (pd->channel) {
    case TIM_CHANNEL_1:
      running = running && ((((TIM_HandleTypeDef *)pd->instance)->Instance->CCER & TIM_CCER_CC1E) != 0);
      break;
    case TIM_CHANNEL_2:
      running = running && ((((TIM_HandleTypeDef *)pd->instance)->Instance->CCER & TIM_CCER_CC2E) != 0);
      break;
    case TIM_CHANNEL_3:
      running = running && ((((TIM_HandleTypeDef *)pd->instance)->Instance->CCER & TIM_CCER_CC3E) != 0);
      break;
    case TIM_CHANNEL_4:
      running = running && ((((TIM_HandleTypeDef *)pd->instance)->Instance->CCER & TIM_CCER_CC4E) != 0);
      break;
    default:
      return EINVAL;
  }

  *rt_running_ptr = running;

  return ESUCCESS;
}

static errno_t start(const Device_PWM *const pd) {
  if (pd == NULL) return EINVAL;

  HAL_StatusTypeDef status = HAL_TIM_PWM_Start((TIM_HandleTypeDef *)pd->instance, pd->channel);

  return status == HAL_OK ? ESUCCESS : EINTR;
}

static errno_t stop(const Device_PWM *const pd) {
  if (pd == NULL) return EINVAL;

  HAL_StatusTypeDef status = HAL_TIM_PWM_Stop((TIM_HandleTypeDef *)pd->instance, pd->channel);

  return status == HAL_OK ? ESUCCESS : EINTR;
}

static errno_t set_prescaler(const Device_PWM *const pd, uint16_t value) {
  __HAL_TIM_SET_PRESCALER((TIM_HandleTypeDef *)pd->instance, value);
  return ESUCCESS;
}

static errno_t set_clock_division(const Device_PWM *const pd, uint8_t value) {
  switch (value) {
    case 1:
      __HAL_TIM_SET_CLOCKDIVISION((TIM_HandleTypeDef *)pd->instance, TIM_CLOCKDIVISION_DIV1);
      return ESUCCESS;
    case 2:
      __HAL_TIM_SET_CLOCKDIVISION((TIM_HandleTypeDef *)pd->instance, TIM_CLOCKDIVISION_DIV2);
      return ESUCCESS;
    case 4:
      __HAL_TIM_SET_CLOCKDIVISION((TIM_HandleTypeDef *)pd->instance, TIM_CLOCKDIVISION_DIV4);
      return ESUCCESS;
  }

  return EINVAL;
}

static errno_t set_auto_reload_register(const Device_PWM *const pd, uint32_t value) {
  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->instance;

  // tim2 和 tim5 的 AutoReload Register 寄存器有 32 位, 可以完整保存 value,
  // 其余只能保存 16 位, 需要判断范围
  if ((htim != &htim2) && value > 0xFFFF) return EINVAL;

  __HAL_TIM_SET_AUTORELOAD(htim, value);

  return ESUCCESS;
}

static errno_t set_compare(const Device_PWM *const pd, uint32_t value) {
  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->instance;

  __HAL_TIM_SET_COMPARE(htim, pd->channel, value);

  return ESUCCESS;
}

static errno_t get_source_frequent(const Device_PWM *const pd, uint32_t *rt_frequent_ptr) {
  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->instance;

  if (htim == &htim3) {
    *rt_frequent_ptr = HAL_RCC_GetPCLK1Freq() * 2;
    return ESUCCESS;
  } else if (htim == &htim1 || htim == &htim8) {
    *rt_frequent_ptr = HAL_RCC_GetPCLK2Freq() * 2;
    return ESUCCESS;
  }

  return EINVAL;
}
