#pragma once

#include "common/errno/errno.h"
#include <stdint.h>

typedef enum {
  DEVICE_RTC_1,
  DEVICE_RTC_COUNT,
} Device_RTC_name;

typedef enum {
  DEVICE_RTC_DR_0,
  DEVICE_RTC_DR_1,
  DEVICE_RTC_DR_2,
  DEVICE_RTC_DR_COUNT,
} Device_RTC_DR_name;

struct Device_RTC;
struct Device_RTC_ops;

typedef struct {
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
} Device_RTC_date;

typedef struct {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} Device_RTC_time;

typedef struct {
  Device_RTC_date date;
  Device_RTC_time time;
} Device_RTC_date_time;

typedef struct Device_RTC {
  const Device_RTC_name name;
  void *const instance;
  const struct Device_RTC_ops *ops;
} Device_RTC;

typedef struct Device_RTC_ops {
  errno_t (*init)(Device_RTC *const pd);
  errno_t (*get_date_time)(Device_RTC *const pd, Device_RTC_date_time *rt_dt_ptr);
  errno_t (*set_date_time)(Device_RTC *const pd, Device_RTC_date_time *dt_ptr);
} Device_RTC_ops;

typedef struct Driver_RTC_ops {
  errno_t (*get_date_time)(Device_RTC *const pd, Device_RTC_date_time *rt_dt_ptr);
  errno_t (*set_date_time)(Device_RTC *const pd, Device_RTC_date_time *dt_ptr);
  errno_t (*get_bkp_dr)(Device_RTC *const pd, Device_RTC_DR_name dr_name, uint32_t *rt_data_ptr);
  errno_t (*set_bkp_dr)(Device_RTC *const pd, Device_RTC_DR_name dr_name, uint32_t data);
} Driver_RTC_ops;

errno_t Device_RTC_module_init(void);
errno_t Device_RTC_register(Device_RTC *const pd);
errno_t Device_RTC_find(Device_RTC **pd_ptr, const Device_RTC_name name);
