#include "timer.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "Core/Inc/tim.h"
#include "device/timer/timer.h"

static errno_t is_running(const Device_timer *const pd, bool *rt_running_ptr);
static errno_t start(const Device_timer *const pd, Device_timer_start_mode mode);
static errno_t stop(const Device_timer *const pd);
static errno_t get_register_count(const Device_timer *const pd, uint32_t *rt_count_ptr);
static errno_t get_count(const Device_timer *const pd, uint32_t *rt_count_ptr);
static errno_t set_prescaler(const Device_timer *const pd, uint16_t value);
static errno_t set_clock_division(const Device_timer *const pd, uint8_t value);
static errno_t set_auto_reload_register(const Device_timer *const pd, uint32_t value);
static errno_t get_source_frequent(const Device_timer *const pd, uint32_t *rt_frequent_ptr);

static const Driver_timer_ops ops = {
  .is_running = is_running,
  .start = start,
  .stop = stop,
  .get_register_count = get_register_count,
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
      *rt_running_ptr = (((SysTick_Type *)pd->instance)->CTRL & SysTick_CTRL_TICKINT_Msk) != 0;
      return ESUCCESS;
    case DEVICE_TIMER_TYPE_GENERAL:
      *rt_running_ptr = (((TIM_HandleTypeDef *)pd->instance)->Instance->CR1 & TIM_CR1_CEN) != 0;
      return ESUCCESS;
  }

  return EINVAL;
}

static errno_t start(const Device_timer *const pd, Device_timer_start_mode mode) {
  if (pd == NULL) return EINVAL;

  switch (pd->type) {
    case DEVICE_TIMER_TYPE_SYSTICK: {
      HAL_ResumeTick();
      return ESUCCESS;
    }
    case DEVICE_TIMER_TYPE_GENERAL: {
      HAL_StatusTypeDef status;

      switch (mode) {
        case DEVICE_TIMER_START_MODE_IT: {
          status = HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *)pd->instance);
          break;
        }
        case DEVICE_TIMER_START_MODE_NO_IT: {
          status = HAL_TIM_Base_Start((TIM_HandleTypeDef *)pd->instance);
          break;
        }
        default: {
          return EINVAL;
        }
      }

      return status == HAL_OK ? ESUCCESS : EINTR;
    }
  }

  return EINVAL;
}

static errno_t stop(const Device_timer *const pd) {
  if (pd == NULL) return EINVAL;

  switch (pd->type) {
    case DEVICE_TIMER_TYPE_SYSTICK: {
      HAL_SuspendTick();
      return ESUCCESS;
    }
    case DEVICE_TIMER_TYPE_GENERAL: {
      TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->instance;
      HAL_StatusTypeDef status;
      if (htim->Instance->DIER & TIM_DIER_UIE) {
        // 通过 HAL_TIM_Base_Start_IT 开启的, 以 HAL_TIM_Base_Stop_IT 关闭
        status = HAL_TIM_Base_Stop_IT(htim);
      } else {
        // 通过 HAL_TIM_Base_Start 开启的, 以 HAL_TIM_Base_Stop 关闭
        status = HAL_TIM_Base_Stop(htim);
      }
      return status == HAL_OK ? ESUCCESS : EINTR;
    }
  }
  
  return EINVAL;
}

static errno_t get_register_count(const Device_timer *const pd, uint32_t *rt_count_ptr) {
  if (pd == NULL) return EINVAL;

  if (pd->type == DEVICE_TIMER_TYPE_GENERAL) {
    TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->instance;
    *rt_count_ptr = __HAL_TIM_GET_COUNTER(htim);
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

  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->instance;

  __HAL_TIM_SET_PRESCALER(htim, value);

  // 如果定时器还未启动, 生成一次更新事件，确保预装载(ARPE=1 时)立即生效
  if ((htim->Instance->CR1 & TIM_CR1_CEN) == 0) {
    __HAL_TIM_SET_COUNTER(htim, 0);
    HAL_StatusTypeDef status = HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE);;
    if (status != HAL_OK) return EINTR;
  }

  return ESUCCESS;
}

static errno_t set_clock_division(const Device_timer *const pd, uint8_t value) {
  if (pd == NULL || pd->type != DEVICE_TIMER_TYPE_GENERAL) return EINVAL;

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

static errno_t set_auto_reload_register(const Device_timer *const pd, uint32_t value) {
  if (pd == NULL || pd->type != DEVICE_TIMER_TYPE_GENERAL) return EINVAL;

  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->instance;

  // tim2 和 tim5 的 AutoReload Register 寄存器有 32 位, 可以完整保存 value,
  // 其余只能保存 16 位, 需要判断范围
  if ((htim != &htim2) && value > 0xFFFF) return EINVAL;

  __HAL_TIM_SET_AUTORELOAD(htim, value);

  // 如果定时器还未启动, 生成一次更新事件，确保预装载(ARPE=1 时)立即生效
  if ((htim->Instance->CR1 & TIM_CR1_CEN) == 0) {
    __HAL_TIM_SET_COUNTER(htim, 0);
    HAL_StatusTypeDef status = HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE);
    if (status != HAL_OK) return EINTR;
  }

  return ESUCCESS;
}

static errno_t get_source_frequent(const Device_timer *const pd, uint32_t *rt_frequent_ptr) {
  if (pd == NULL || pd->type != DEVICE_TIMER_TYPE_GENERAL) return EINVAL;

  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)pd->instance;

  if (htim == &htim2 || htim == &htim6 || htim == &htim7) {
    *rt_frequent_ptr = HAL_RCC_GetPCLK1Freq() * 2;
    return ESUCCESS;
  } else if (htim == &htim10 || htim == &htim11) {
    *rt_frequent_ptr = HAL_RCC_GetPCLK2Freq() * 2;
    return ESUCCESS;
  }

  return EINVAL;
}

errno_t Driver_timer_get_ops(const Driver_timer_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}
