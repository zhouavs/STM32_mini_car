#include "rtc.h"
#include "common/list/list.h"
#include "common/ring_buffer/ring_buffer.h"
#include "driver/rtc/rtc.h"
#include <stdlib.h>

#define RTC_SETTED_TIME 0x1234

static errno_t init(Device_RTC *const pd);
static errno_t get_date_time(Device_RTC *const pd, Device_RTC_date_time *rt_dt_ptr);
static errno_t set_date_time(Device_RTC *const pd, Device_RTC_date_time *dt_ptr);

static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_RTC_ops device_ops = {
  .init = init,
  .get_date_time = get_date_time,
  .set_date_time = set_date_time,
};

static List *list = NULL;
static const Driver_RTC_ops *driver_ops = NULL;

errno_t Device_RTC_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_RTC_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_RTC_register(Device_RTC *const pd) {
  if (list == NULL || pd == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, pd);
}

errno_t Device_RTC_find(Device_RTC **pd_ptr, const Device_RTC_name name) {
  if (list == NULL) return EINVAL;
  return list->ops->find(list, pd_ptr, &name, match_device_by_name);
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return (((Device_RTC *)pd)->name == *(Device_RTC_name *)name);
}

static errno_t init(Device_RTC *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  uint32_t setted_flag = 0;
  err = driver_ops->get_bkp_dr(pd, DEVICE_RTC_DR_0, &setted_flag);
  if (err) return err;

  if (setted_flag == RTC_SETTED_TIME) return ESUCCESS;

  Device_RTC_date_time init_dt = {
    .date = {
      .year = 25,
      .month = 11,
      .day = 29,
      .weekday = 6,
    },
    .time = {
      .hour = 20,
      .minute = 40,
      .second = 34,
    },
  };

  err = driver_ops->set_date_time(pd, &init_dt);
  if (err) return err;

  err = driver_ops->set_bkp_dr(pd, DEVICE_RTC_DR_0, RTC_SETTED_TIME);
  if (err) return err;

  return ESUCCESS;
}

static errno_t get_date_time(Device_RTC *const pd, Device_RTC_date_time *rt_dt_ptr) {
  return driver_ops->get_date_time(pd, rt_dt_ptr);
}
static errno_t set_date_time(Device_RTC *const pd, Device_RTC_date_time *dt_ptr) {
  errno_t err = ESUCCESS;

  err = driver_ops->set_date_time(pd, dt_ptr);
  if (err) return err;

  err = driver_ops->set_bkp_dr(pd, DEVICE_RTC_DR_0, RTC_SETTED_TIME);
  if (err) return err;

  return ESUCCESS;
}
