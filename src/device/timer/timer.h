#pragma once

#include "common/errno/errno.h"
#include "stdint.h"
#include "stdbool.h"

typedef enum {
  DEVICE_TIMER_SYSTICK,
  DEVICE_TIMER_TIM2,
  DEVICE_TIMER_TIM6,
  DEVICE_TIMER_COUNT,
} Device_timer_name;

typedef enum {
  DEVICE_TIMER_TYPE_SYSTICK,
  DEVICE_TIMER_TYPE_GENERAL,
} Device_timer_type;

struct Device_timer;
struct Device_timer_ops;

typedef struct Device_timer {
  const Device_timer_name name;
  const Device_timer_type type;
  const void *const instance;
  const struct Device_timer_ops *ops;
} Device_timer;

typedef struct Device_timer_ops {
  errno_t (*init)(const Device_timer *const pd);
  errno_t (*is_running)(const Device_timer *const pd, bool *rt_running_ptr);
  errno_t (*start)(const Device_timer *const pd);
  errno_t (*stop)(const Device_timer *const pd);
  errno_t (*get_count)(const Device_timer *const pd, uint32_t *rt_count_ptr);
  errno_t (*set_preiod)(const Device_timer *const pd, uint32_t us);
  errno_t (*set_prescaler)(const Device_timer *const pd, uint16_t value);
  errno_t (*set_clock_division)(const Device_timer *const pd, uint8_t value);
  errno_t (*set_auto_reload_register)(const Device_timer *const pd, uint32_t value);
} Device_timer_ops;

typedef struct Driver_timer_ops {
  errno_t (*is_running)(const Device_timer *const pd, bool *rt_running_ptr);
  errno_t (*start)(const Device_timer *const pd);
  errno_t (*stop)(const Device_timer *const pd);
  errno_t (*get_count)(const Device_timer *const pd, uint32_t *rt_count_ptr);
  errno_t (*set_prescaler)(const Device_timer *const pd, uint16_t value);
  errno_t (*set_clock_division)(const Device_timer *const pd, uint8_t value);
  errno_t (*set_auto_reload_register)(const Device_timer *const pd, uint32_t value);
  errno_t (*get_source_frequent)(const Device_timer *const pd, uint32_t *rt_frequent_ptr);
} Driver_timer_ops;

errno_t Device_timer_module_init(void);
errno_t Device_timer_register(Device_timer *const pd);
errno_t Device_timer_find(Device_timer **pd_ptr, const Device_timer_name name);

errno_t Device_timer_PeriodElapsedCallback(const Device_timer *const pd);
