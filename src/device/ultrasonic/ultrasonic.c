#include "ultrasonic.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 对象方法
static errno_t init(Device_ultrasonic *const pd);
static errno_t read(Device_ultrasonic *const pd, uint32_t *rt_data_ptr);

// 内部方法
// 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_ultrasonic_ops device_ops = {
  .init = init,
  .read = read,
};

static List *list = NULL;

errno_t Device_ultrasonic_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_ultrasonic_register(Device_ultrasonic *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_ultrasonic_find(Device_ultrasonic **pd_ptr, const Device_ultrasonic_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(Device_ultrasonic *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->timer->ops->init(pd->timer);
  if (err) return err;
  
  // 计时单位为 1 微秒
  err = pd->timer->ops->set_time_for_count_incr(pd->timer, 1);
  if (err) return err;
  // 溢出值设置为最大(为了保持兼容, 设置 0xFFFF)
  err = pd->timer->ops->set_auto_reload_register(pd->timer, 0xFFFF);
  if (err) return err;
  
  err = pd->timer->ops->start(pd->timer, DEVICE_TIMER_START_MODE_NO_IT);
  if (err) return err;

  err = pd->trig->ops->init(pd->trig);
  if (err) return err;

  err = pd->echo->ops->init(pd->echo);
  if (err) return err;

  return ESUCCESS;
}

static errno_t read(Device_ultrasonic *const pd, uint32_t *rt_data_ptr) {
  if (pd == NULL) return EINVAL;


}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_ultrasonic *)pd)->name == *((Device_ultrasonic_name *)name);
}
