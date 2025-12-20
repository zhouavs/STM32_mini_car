#pragma once

#include "common/errno/errno.h"
#include "device/timer/timer.h"
#include <stdint.h>

typedef enum {
  DEVICE_DAC_LIGHT,
  DEVICE_DAC_COUNT,
} Device_DAC_name;

/**
 * @brief 通道
 */
typedef enum {
  DEVICE_DAC_CHANNEL_1,
  DEVICE_DAC_CHANNEL_2,
} Device_DAC_channel;

typedef enum {
  DEVICE_DAC_ALIGN_12B_R,
  DEVICE_DAC_ALIGN_12B_L,
  DEVICE_DAC_ALIGN_8B_R
} Device_DAC_align;

typedef enum {
  DEVICE_DAC_OUTPUT_BUFFER_ENABLE,
  DEVICE_DAC_OUTPUT_BUFFER_DISABLE,
} Device_DAC_output_buffer;

typedef enum {
  DEVICE_DAC_TRIGGER_NONE,
  DEVICE_DAC_TRIGGER_T2_TRGO,
  DEVICE_DAC_TRIGGER_T4_TRGO,
  DEVICE_DAC_TRIGGER_T5_TRGO,
  DEVICE_DAC_TRIGGER_T6_TRGO,
  DEVICE_DAC_TRIGGER_T7_TRGO,
  DEVICE_DAC_TRIGGER_T8_TRGO,
  DEVICE_DAC_TRIGGER_EXT_IT9,
  DEVICE_DAC_TRIGGER_SOFTWARE,
} Device_DAC_trigger;

typedef enum {
  DEVICE_DAC_RUN_STATUS_NONE,
  DEVICE_DAC_RUN_STATUS_POINT,
  DEVICE_DAC_RUN_STATUS_WAVE,
} Device_DAC_run_status;

typedef struct {
  Device_DAC_trigger trigger;
  Device_DAC_output_buffer output_buffer;
} Device_DAC_channel_config;

struct Device_DAC;
struct Device_DAC_ops;

typedef struct Device_DAC {
  const Device_DAC_name name;
  Device_DAC_run_status run_status;
  void *const instance;
  const Device_DAC_channel channel;
  Device_timer *timer;
  const struct Device_DAC_ops *ops;
} Device_DAC;

typedef struct Device_DAC_ops {
  errno_t (*init)(Device_DAC *const pd);
  errno_t (*reset)(Device_DAC *const pd);
  errno_t (*set_point)(Device_DAC *const pd, uint16_t point);
  errno_t (*set_wave)(Device_DAC *const pd, uint16_t *points, uint16_t len, uint16_t refresh_interval_us);
} Device_DAC_ops;

typedef struct Driver_DAC_ops {
  errno_t (*config_channel)(const Device_DAC *const pd, const Device_DAC_channel_config *const config);
  errno_t (*set_value)(const Device_DAC *const pd, uint16_t value, Device_DAC_align align);
  errno_t (*start)(const Device_DAC *const pd);
  errno_t (*stop)(const Device_DAC *const pd);
  errno_t (*start_DMA)(const Device_DAC *const pd, const uint16_t *data, uint16_t len, Device_DAC_align align);
  errno_t (*stop_DMA)(const Device_DAC *const pd);
} Driver_DAC_ops;

errno_t Device_DAC_module_init(void);
errno_t Device_DAC_register(Device_DAC *const pd);
errno_t Device_DAC_find(Device_DAC **pd_ptr, const Device_DAC_name name);
