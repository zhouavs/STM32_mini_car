#pragma once

#include "common/errno/errno.h"
#include "stdint.h"
#include "stdbool.h"

typedef enum {
  DEVICE_PWM_TIM_1_CH_1,
  DEVICE_PWM_TIM_3_CH_3,
  DEVICE_PWM_TIM_8_CH_1,
  DEVICE_PWM_TIM_8_CH_2,
  DEVICE_PWM_TIM_8_CH_3,
  DEVICE_PWM_TIM_8_CH_4,
  DEVICE_PWM_COUNT,
} Device_PWM_name;

struct Device_PWM;
struct Device_PWM_ops;

typedef struct Device_PWM {
  const Device_PWM_name name;
  const void *const instance;
  const uint8_t channel;
  const struct Device_PWM_ops *ops;
} Device_PWM;

typedef struct Device_PWM_ops {
  errno_t (*init)(const Device_PWM *const pd);
  errno_t (*is_running)(const Device_PWM *const pd, bool *rt_running_ptr);
  errno_t (*start)(const Device_PWM *const pd);
  errno_t (*stop)(const Device_PWM *const pd);
  errno_t (*set_preiod)(const Device_PWM *const pd, uint32_t pre_us, uint32_t total_us);
} Device_PWM_ops;

typedef struct Driver_pwm_ops {
  errno_t (*is_running)(const Device_PWM *const pd, bool *rt_running_ptr);
  errno_t (*start)(const Device_PWM *const pd);
  errno_t (*stop)(const Device_PWM *const pd);
  errno_t (*set_prescaler)(const Device_PWM *const pd, uint16_t value);
  errno_t (*set_clock_division)(const Device_PWM *const pd, uint8_t value);
  errno_t (*set_auto_reload_register)(const Device_PWM *const pd, uint32_t value);
  errno_t (*set_compare)(const Device_PWM *const pd, uint32_t value);
  errno_t (*get_source_frequent)(const Device_PWM *const pd, uint32_t *rt_frequent_ptr);
} Driver_pwm_ops;

errno_t Device_PWM_module_init(void);
errno_t Device_PWM_register(Device_PWM *const pd);
errno_t Device_PWM_find(const Device_PWM **pd_ptr, const Device_PWM_name name);

errno_t Device_PWM_PeriodElapsedCallback(const Device_PWM *const pd);
