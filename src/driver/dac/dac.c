#include "dac.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "device/dac/dac.h"

static errno_t config_channel(const Device_DAC *const pd, const Device_DAC_channel_config *const config);
static errno_t start(const Device_DAC *const pd);
static errno_t stop(const Device_DAC *const pd);
static errno_t set_value(const Device_DAC *const pd, uint16_t value, Device_DAC_align align);
static errno_t start_DMA(const Device_DAC *const pd, const uint16_t *data, uint16_t len, Device_DAC_align align);
static errno_t stop_DMA(const Device_DAC *const pd);

static const uint32_t relate_channel[] = {
  [DEVICE_DAC_CHANNEL_1] = DAC_CHANNEL_1,
  [DEVICE_DAC_CHANNEL_2] = DAC_CHANNEL_2,
};

static const uint32_t relate_trigger[] = {
  [DEVICE_DAC_TRIGGER_NONE] = DAC_TRIGGER_NONE,
  [DEVICE_DAC_TRIGGER_T2_TRGO] = DAC_TRIGGER_T2_TRGO,
  [DEVICE_DAC_TRIGGER_T4_TRGO] = DAC_TRIGGER_T4_TRGO,
  [DEVICE_DAC_TRIGGER_T5_TRGO] = DAC_TRIGGER_T5_TRGO,
  [DEVICE_DAC_TRIGGER_T6_TRGO] = DAC_TRIGGER_T6_TRGO,
  [DEVICE_DAC_TRIGGER_T7_TRGO] = DAC_TRIGGER_T7_TRGO,
  [DEVICE_DAC_TRIGGER_T8_TRGO] = DAC_TRIGGER_T8_TRGO,
  [DEVICE_DAC_TRIGGER_EXT_IT9] = DAC_TRIGGER_EXT_IT9,
  [DEVICE_DAC_TRIGGER_SOFTWARE] = DAC_TRIGGER_SOFTWARE,
};

static const uint32_t relate_buffer[] = {
  [DEVICE_DAC_OUTPUT_BUFFER_ENABLE] = DAC_OUTPUTBUFFER_ENABLE,
  [DEVICE_DAC_OUTPUT_BUFFER_DISABLE] = DAC_OUTPUTBUFFER_DISABLE,
};

static const uint32_t relate_align[] = {
  [DEVICE_DAC_ALIGN_12B_R] = DAC_ALIGN_12B_R,
  [DEVICE_DAC_ALIGN_12B_L] = DAC_ALIGN_12B_L,
  [DEVICE_DAC_ALIGN_8B_R] = DAC_ALIGN_8B_R,
};

static const Driver_DAC_ops ops = {
  .config_channel = config_channel,
  .set_value = set_value,
  .start = start,
  .stop = stop,
  .start_DMA = start_DMA,
  .stop_DMA = stop_DMA,
};

errno_t Driver_DAC_get_ops(const Driver_DAC_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}

static errno_t config_channel(const Device_DAC *const pd, const Device_DAC_channel_config *const config) {
  if (pd == NULL) return EINVAL;
  DAC_ChannelConfTypeDef hal_config = {
    .DAC_Trigger = relate_trigger[config->trigger],
    .DAC_OutputBuffer = relate_buffer[config->output_buffer],
  };
  HAL_StatusTypeDef status = HAL_DAC_ConfigChannel((DAC_HandleTypeDef *)pd->instance, &hal_config, relate_channel[pd->channel]);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t start(const Device_DAC *const pd) {
  if (pd == NULL) return EINVAL;
  HAL_StatusTypeDef status = HAL_DAC_Start((DAC_HandleTypeDef *)pd->instance, relate_channel[pd->channel]);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t stop(const Device_DAC *const pd) {
  if (pd == NULL) return EINVAL;
  HAL_StatusTypeDef status = HAL_DAC_Stop((DAC_HandleTypeDef *)pd->instance, relate_channel[pd->channel]);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t set_value(const Device_DAC *const pd, uint16_t value, Device_DAC_align align) {
  if (pd == NULL) return EINVAL;
  HAL_StatusTypeDef status = HAL_DAC_SetValue((DAC_HandleTypeDef *)pd->instance, relate_channel[pd->channel], relate_align[align], value);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t start_DMA(const Device_DAC *const pd, const uint16_t *data, uint16_t len, Device_DAC_align align) {
  if (pd == NULL) return EINVAL;
  HAL_StatusTypeDef status = HAL_DAC_Start_DMA((DAC_HandleTypeDef *)pd->instance, relate_channel[pd->channel], (uint32_t *)data, len, relate_align[align]);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}

static errno_t stop_DMA(const Device_DAC *const pd) {
  if (pd == NULL) return EINVAL;
  HAL_StatusTypeDef status = HAL_DAC_Stop_DMA((DAC_HandleTypeDef *)pd->instance, relate_channel[pd->channel]);
  if (status != HAL_OK) return EINTR;
  return ESUCCESS;
}
