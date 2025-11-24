#include "timer.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include "driver/timer/timer.h"
#include <stdlib.h>
#include <string.h>

static errno_t init(const Device_timer *const pd);
static errno_t is_running(const Device_timer *const pd, bool *rt_running_ptr);
static errno_t start(const Device_timer *const pd);
static errno_t stop(const Device_timer *const pd);
static errno_t get_count(const Device_timer *const pd, uint32_t *rt_count_ptr);
static errno_t set_preiod(const Device_timer *const pd, uint32_t value);
static errno_t set_prescaler(const Device_timer *const pd, uint16_t value);
static errno_t set_clock_division(const Device_timer *const pd, uint8_t value);
static errno_t set_auto_reload_register(const Device_timer *const pd, uint32_t value);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_timer_ops device_ops = {
  .init = init,
  .is_running = is_running,
  .start = start,
  .stop = stop,
  .get_count = get_count,
  .set_preiod = set_preiod,
  .set_prescaler = set_prescaler,
  .set_clock_division = set_clock_division,
  .set_auto_reload_register = set_auto_reload_register,
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

static errno_t start(const Device_timer *const pd) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;

  bool running = false;
  errno_t err = is_running(pd, &running);
  if (err) return err;
  if (running) return EBUSY;

  if (pd->type == DEVICE_TIMER_TYPE_GENERAL) {
    timer_count[pd->name] = 0;
  }

  err = driver_ops->start(pd);
  if (err) return err;

  return ESUCCESS;
}

static errno_t stop(const Device_timer *const pd) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->stop(pd);
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

static errno_t set_preiod(const Device_timer *const pd, uint32_t us) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  errno_t err = ESUCCESS;

  uint32_t frequent = 0;
  err = driver_ops->get_source_frequent(pd, &frequent);
  if (err) return err;

  // 预分频之后的周期为半微秒
  // 因为如果周期设置为 1 微秒的话, 在进行微秒级计数的时候 set_auto_reload_register 的值得设置为 us - 1 也就是 0
  // 但设置为 0 时无法不会产生有效的溢出事件, 不会出发 HAL_TIM_PeriodElapsedCallback  回调
  err = driver_ops->set_prescaler(pd, frequent / (1000000 * 2) - 1);
  if (err) return err;

  // 与此同时, 自动装载寄存器也得设置为微秒数 * 2
  err = driver_ops->set_auto_reload_register(pd, us * 2 - 1);
  if (err) return err;

  return ESUCCESS;
}

static errno_t set_prescaler(const Device_timer *const pd, uint16_t value) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->set_prescaler(pd, value);
}

static errno_t set_clock_division(const Device_timer *const pd, uint8_t value) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->set_clock_division(pd, value);
}

static errno_t set_auto_reload_register(const Device_timer *const pd, uint32_t value) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->set_auto_reload_register(pd, value);
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_timer *)pd)->name == *((Device_timer_name *)name);
}
