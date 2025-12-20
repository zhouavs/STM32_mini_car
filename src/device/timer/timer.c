#include "timer.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include "driver/timer/timer.h"
#include <stdlib.h>
#include <string.h>

static errno_t init(const Device_timer *const pd);
static errno_t is_running(const Device_timer *const pd, bool *rt_running_ptr);
static errno_t start(const Device_timer *const pd, Device_timer_start_mode mode);
static errno_t stop(const Device_timer *const pd);
static errno_t get_register_count(const Device_timer *const pd, uint32_t *rt_count_ptr);
static errno_t get_count(const Device_timer *const pd, uint32_t *rt_count_ptr);
static errno_t set_period(const Device_timer *const pd, uint32_t value);
static errno_t set_time_for_count_incr(const Device_timer *const pd, uint32_t us);
static errno_t set_prescaler(const Device_timer *const pd, uint16_t value);
static errno_t set_clock_division(const Device_timer *const pd, uint8_t value);
static errno_t set_auto_reload_register(const Device_timer *const pd, uint32_t value);
static errno_t set_period_elapsed_callback(Device_timer *const pd, Device_timer_callback *callback);
static errno_t get_source_frequent(const Device_timer *const pd, uint32_t *rt_frequent_ptr);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_timer_ops device_ops = {
  .init = init,
  .is_running = is_running,
  .start = start,
  .stop = stop,
  .get_register_count = get_register_count,
  .get_count = get_count,
  .set_period = set_period,
  .set_time_for_count_incr = set_time_for_count_incr,
  .set_prescaler = set_prescaler,
  .set_clock_division = set_clock_division,
  .set_auto_reload_register = set_auto_reload_register,
  .set_period_elapsed_callback = set_period_elapsed_callback,
  .get_source_frequent = get_source_frequent,
};

static const Driver_timer_ops *driver_ops = NULL;
static List *list = NULL;
static volatile uint32_t timer_count[DEVICE_TIMER_COUNT] = {0};

errno_t Device_timer_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_timer_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_timer_register(Device_timer *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_timer_find(Device_timer **pd_ptr, const Device_timer_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

errno_t Device_timer_PeriodElapsedCallback(const Device_timer *const pd) {
  ++timer_count[pd->name];
  // 如果关联了其他回调函数, 执行该回调函数
  if (pd->period_elapsed_callback != NULL) {
    return pd->period_elapsed_callback();
  }
  return ESUCCESS;
}

static errno_t init(const Device_timer *const pd) {
  if (pd == NULL) return EINVAL;

  return ESUCCESS;
}

static errno_t is_running(const Device_timer *const pd, bool *rt_running_ptr) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->is_running(pd, rt_running_ptr);
}

static errno_t start(const Device_timer *const pd, Device_timer_start_mode mode) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;

  bool running = false;
  errno_t err = is_running(pd, &running);
  if (err) return err;
  if (running) return EBUSY;

  if (pd->type == DEVICE_TIMER_TYPE_GENERAL) {
    timer_count[pd->name] = 0;
  }

  err = driver_ops->start(pd, mode);
  if (err) return err;

  return ESUCCESS;
}

static errno_t stop(const Device_timer *const pd) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->stop(pd);
}

static errno_t get_register_count(const Device_timer *const pd, uint32_t *rt_count_ptr) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->get_register_count(pd, rt_count_ptr);
}

static errno_t get_count(const Device_timer *const pd, uint32_t *rt_count_ptr) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;

  switch (pd->type) {
    case DEVICE_TIMER_TYPE_SYSTICK:
      driver_ops->get_count(pd, rt_count_ptr);
      return ESUCCESS;
    case DEVICE_TIMER_TYPE_GENERAL:
      *rt_count_ptr = timer_count[pd->name];
      return ESUCCESS;
  }

  return EINVAL;
}

static errno_t set_period(const Device_timer *const pd, uint32_t us) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  errno_t err = ESUCCESS;

  err = set_time_for_count_incr(pd, 1);
  if (err) return err;

  // 如果传入的 us 参数为 1, 那么 us - 1 会为 0, 此时无法产生溢出回调, 而且 1 微秒级的回调根本不准确, 所以此处不针对传入 1 微秒的情况进行特殊处理, 不要这么做
  err = driver_ops->set_auto_reload_register(pd, us - 1);
  if (err) return err;

  return ESUCCESS;
}

static errno_t set_time_for_count_incr(const Device_timer *const pd, uint32_t us) {
  errno_t err = ESUCCESS;

  uint32_t frequent = 0;
  err = driver_ops->get_source_frequent(pd, &frequent);
  if (err) return err;

  const uint32_t div = (1000000 / us);

  if (frequent % div != 0) {
    return EINVAL;
  }

  const uint32_t psc = frequent / div;

  if (psc == 0 || psc > 0x10000) return EINVAL;

  err = driver_ops->set_prescaler(pd, (uint16_t)(psc - 1));
  if (err) return err;

  return ESUCCESS;
}

static errno_t set_prescaler(const Device_timer *const pd, uint16_t value) {
  if (pd == NULL) return EINVAL;
  return driver_ops->set_prescaler(pd, value);
}

static errno_t set_clock_division(const Device_timer *const pd, uint8_t value) {
  if (pd == NULL) return EINVAL;
  return driver_ops->set_clock_division(pd, value);
}

static errno_t set_auto_reload_register(const Device_timer *const pd, uint32_t value) {
  if (pd == NULL) return EINVAL;
  return driver_ops->set_auto_reload_register(pd, value);
}

static errno_t set_period_elapsed_callback(Device_timer *const pd, Device_timer_callback *callback) {
  if (pd == NULL || callback == NULL) return EINVAL;
  pd->period_elapsed_callback = callback;
  return ESUCCESS;
}

static errno_t get_source_frequent(const Device_timer *const pd, uint32_t *rt_frequent_ptr) {
  if (pd == NULL) return EINVAL;
  return driver_ops->get_source_frequent(pd, rt_frequent_ptr);
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_timer *)pd)->name == *((Device_timer_name *)name);
}
