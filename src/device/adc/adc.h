#pragma once

#include "common/errno/errno.h"
#include <stdint.h>

typedef enum {
  DEVICE_ADC_LIGHT,
  DEVICE_ADC_POWER,
  DEVICE_ADC_COUNT,
} Device_ADC_name;

/**
 * @brief 通道
 */
typedef enum {
  DEVICE_ADC_CHANNEL_0,
  DEVICE_ADC_CHANNEL_1,
  DEVICE_ADC_CHANNEL_2,
  DEVICE_ADC_CHANNEL_3,
  DEVICE_ADC_CHANNEL_4,
  DEVICE_ADC_CHANNEL_5,
  DEVICE_ADC_CHANNEL_6,
  DEVICE_ADC_CHANNEL_7,
  DEVICE_ADC_CHANNEL_8,
  DEVICE_ADC_CHANNEL_9,
  DEVICE_ADC_CHANNEL_10,
  DEVICE_ADC_CHANNEL_11,
  DEVICE_ADC_CHANNEL_12,
  DEVICE_ADC_CHANNEL_13,
  DEVICE_ADC_CHANNEL_14,
  DEVICE_ADC_CHANNEL_15,
  DEVICE_ADC_CHANNEL_16,
  DEVICE_ADC_CHANNEL_17,
  DEVICE_ADC_CHANNEL_18,
} Device_ADC_channel;

/**
 * @brief 采样时间
 */
typedef enum {
  DEVICE_ADC_SAMPLING_TIME_3_CYCLES,
  DEVICE_ADC_SAMPLING_TIME_15_CYCLES,
  DEVICE_ADC_SAMPLING_TIME_28_CYCLES,
  DEVICE_ADC_SAMPLING_TIME_56_CYCLES,
  DEVICE_ADC_SAMPLING_TIME_84_CYCLES,
  DEVICE_ADC_SAMPLING_TIME_112_CYCLES,
  DEVICE_ADC_SAMPLING_TIME_144_CYCLES,
  DEVICE_ADC_SAMPLING_TIME_480_CYCLES,
} Device_ADC_sampling_time;

typedef struct {
  Device_ADC_channel channel;
  Device_ADC_sampling_time sampling_time;
  uint8_t rank;
} Device_ADC_channel_config;

struct Device_ADC;
struct Device_ADC_ops;

typedef struct Device_ADC {
  const Device_ADC_name name;
  void *const instance;
  const Device_ADC_channel channel;
  const Device_ADC_sampling_time sampling_time;
  const struct Device_ADC_ops *ops;
} Device_ADC;

typedef struct Device_ADC_ops {
  errno_t (*init)(Device_ADC *const pd);
  errno_t (*read)(Device_ADC *const pd, uint16_t *rt_data, uint32_t len);
} Device_ADC_ops;

typedef struct Driver_ADC_ops {
  errno_t (*config_channel)(const Device_ADC *const pd, const Device_ADC_channel_config *const config);
  errno_t (*start)(const Device_ADC *const pd);
  errno_t (*stop)(const Device_ADC *const pd);
  errno_t (*poll)(const Device_ADC *const pd, uint32_t timeout_ms);
  errno_t (*get_value)(const Device_ADC *const pd, uint16_t *rt_value);
} Driver_ADC_ops;

errno_t Device_ADC_module_init(void);
errno_t Device_ADC_register(Device_ADC *const pd);
errno_t Device_ADC_find(Device_ADC **pd_ptr, const Device_ADC_name name);
