#include "adc.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "device/adc/adc.h"

static errno_t config_channel(const Device_ADC *const pd, const Device_ADC_channel_config *const config);
static errno_t start(const Device_ADC *const pd);
static errno_t stop(const Device_ADC *const pd);
static errno_t poll(const Device_ADC *const pd, uint32_t timeout_ms);
static errno_t get_value(const Device_ADC *const pd, uint16_t *rt_value);

static const uint32_t relate_channel[] = {
  [DEVICE_ADC_CHANNEL_0] = ADC_CHANNEL_0,
  [DEVICE_ADC_CHANNEL_1] = ADC_CHANNEL_1,
  [DEVICE_ADC_CHANNEL_2] = ADC_CHANNEL_2,
  [DEVICE_ADC_CHANNEL_3] = ADC_CHANNEL_3,
  [DEVICE_ADC_CHANNEL_4] = ADC_CHANNEL_4,
  [DEVICE_ADC_CHANNEL_5] = ADC_CHANNEL_5,
  [DEVICE_ADC_CHANNEL_6] = ADC_CHANNEL_6,
  [DEVICE_ADC_CHANNEL_7] = ADC_CHANNEL_7,
  [DEVICE_ADC_CHANNEL_8] = ADC_CHANNEL_8,
  [DEVICE_ADC_CHANNEL_9] = ADC_CHANNEL_9,
  [DEVICE_ADC_CHANNEL_10] = ADC_CHANNEL_10,
  [DEVICE_ADC_CHANNEL_11] = ADC_CHANNEL_11,
  [DEVICE_ADC_CHANNEL_12] = ADC_CHANNEL_12,
  [DEVICE_ADC_CHANNEL_13] = ADC_CHANNEL_13,
  [DEVICE_ADC_CHANNEL_14] = ADC_CHANNEL_14,
  [DEVICE_ADC_CHANNEL_15] = ADC_CHANNEL_15,
  [DEVICE_ADC_CHANNEL_16] = ADC_CHANNEL_16,
  [DEVICE_ADC_CHANNEL_17] = ADC_CHANNEL_17,
  [DEVICE_ADC_CHANNEL_18] = ADC_CHANNEL_18,
};

static const uint32_t relate_sampling[] = {
  [DEVICE_ADC_SAMPLING_TIME_3_CYCLES] = ADC_SAMPLETIME_3CYCLES,
  [DEVICE_ADC_SAMPLING_TIME_15_CYCLES] = ADC_SAMPLETIME_15CYCLES,
  [DEVICE_ADC_SAMPLING_TIME_28_CYCLES] = ADC_SAMPLETIME_28CYCLES,
  [DEVICE_ADC_SAMPLING_TIME_56_CYCLES] = ADC_SAMPLETIME_56CYCLES,
  [DEVICE_ADC_SAMPLING_TIME_84_CYCLES] = ADC_SAMPLETIME_84CYCLES,
  [DEVICE_ADC_SAMPLING_TIME_112_CYCLES] = ADC_SAMPLETIME_112CYCLES,
  [DEVICE_ADC_SAMPLING_TIME_144_CYCLES] = ADC_SAMPLETIME_144CYCLES,
  [DEVICE_ADC_SAMPLING_TIME_480_CYCLES] = ADC_SAMPLETIME_480CYCLES,
};

static const Driver_ADC_ops ops = {
  .config_channel = config_channel,
  .start = start,
  .stop = stop,
  .poll = poll,
  .get_value = get_value,
};

errno_t Driver_ADC_get_ops(const Driver_ADC_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}

static errno_t config_channel(const Device_ADC *const pd, const Device_ADC_channel_config *const config) {
  if (pd == NULL || config == NULL) return EINVAL;
  ADC_ChannelConfTypeDef hal_config = {
    .Channel = relate_channel[config->channel],
    .SamplingTime = relate_sampling[config->sampling_time],
    .Rank = config->rank,
  };
  HAL_StatusTypeDef status = HAL_ADC_ConfigChannel((ADC_HandleTypeDef *)pd->instance, &hal_config);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t start(const Device_ADC *const pd) {
  if (pd == NULL) return EINVAL;
  HAL_StatusTypeDef status = HAL_ADC_Start((ADC_HandleTypeDef *)pd->instance);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t stop(const Device_ADC *const pd) {
  if (pd == NULL) return EINVAL;
  HAL_StatusTypeDef status = HAL_ADC_Stop((ADC_HandleTypeDef *)pd->instance);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t poll(const Device_ADC *const pd, uint32_t timeout_ms) {
  if (pd == NULL) return EINVAL;
  HAL_StatusTypeDef status = HAL_ADC_PollForConversion((ADC_HandleTypeDef *)pd->instance, timeout_ms);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t get_value(const Device_ADC *const pd, uint16_t *rt_value) {
  if (pd == NULL || rt_value == NULL) return EINVAL;
  uint16_t value = (uint16_t)HAL_ADC_GetValue((ADC_HandleTypeDef *)pd->instance);
  *rt_value = value;
  return ESUCCESS;
}

