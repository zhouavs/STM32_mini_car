#include "pwm.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include "driver/pwm/pwm.h"
#include <stdlib.h>
#include <string.h>

static errno_t init(const Device_PWM *const pd);
static errno_t is_running(const Device_PWM *const pd, bool *rt_running_ptr);
static errno_t start(const Device_PWM *const pd);
static errno_t stop(const Device_PWM *const pd);
static errno_t set_period(const Device_PWM *const pd, uint32_t up_us, uint32_t total_us);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_PWM_ops device_ops = {
  .init = init,
  .is_running = is_running,
  .start = start,
  .stop = stop,
  .set_period = set_period,
};

static const Driver_pwm_ops *driver_ops = NULL;
static List *list = NULL;

errno_t Device_PWM_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_pwm_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_PWM_register(Device_PWM *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_PWM_find(const Device_PWM **pd_ptr, const Device_PWM_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(const Device_PWM *const pd) {
  if (pd == NULL) return EINVAL;

  return ESUCCESS;
}

static errno_t is_running(const Device_PWM *const pd, bool *rt_running_ptr) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->is_running(pd, rt_running_ptr);
}

static errno_t start(const Device_PWM *const pd) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;

  bool running = false;
  errno_t err = is_running(pd, &running);
  if (err) return err;
  if (running) return EBUSY;

  err = driver_ops->start(pd);
  if (err) return err;

  return ESUCCESS;
}

static errno_t stop(const Device_PWM *const pd) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  return driver_ops->stop(pd);
}

static errno_t set_period(const Device_PWM *const pd, uint32_t pre_us, uint32_t total_us) {
  if (pd == NULL || driver_ops == NULL) return EINVAL;
  errno_t err = ESUCCESS;

  uint32_t frequent = 0;
  err = driver_ops->get_source_frequent(pd, &frequent);
  if (err) return err;

  // 频率单位更改为 MHz, 避免后续计算中数据太大导致溢出
  frequent /= 1000000;
  // 需要的总计数量
  uint32_t total_count = total_us * frequent;
  // 需要设置的分频, 因为计数寄存器最大只有 0xFFFF, 如果需要设定大于 0xFFFF 个计数周期, 需要依赖分频实现
  const uint32_t prescaler = (total_count >> 16) + 1;
  // 分频后还需要的计数周期
  total_count /= prescaler;
  // 配置 pwm 波前半段电平占比的比较数 
  // 公式为 pre_count = (pre_us / total_us) * total_count
  const uint32_t pre_count = (uint32_t)(((uint64_t)pre_us * total_count) / total_us); // 防止先除后乘丢失精度

  err = driver_ops->set_prescaler(pd, prescaler - 1);
  if (err) return err;

  err = driver_ops->set_auto_reload_register(pd, total_count - 1);
  if (err) return err;

  // 小于比较值为前半段电平, 不需要减 1
  err = driver_ops->set_compare(pd, pre_count);
  if (err) return err;

  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_PWM *)pd)->name == *((Device_PWM_name *)name);
}
