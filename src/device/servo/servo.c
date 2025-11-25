#include "servo.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <string.h>

// 对象方法
static errno_t init(Device_servo *const pd);
static errno_t set_angle(Device_servo *const pd, const angle_t angle);
static errno_t start(Device_servo *const pd);
static errno_t stop(Device_servo *const pd);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_servo_ops device_ops = {
  .init = init,
  .set_angle = set_angle,
  .start = start,
  .stop = stop,
};

static List *list = NULL;

errno_t Device_servo_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_servo_register(Device_servo *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_servo_find(Device_servo **pd_ptr, const Device_servo_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(Device_servo *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->pwm->ops->init(pd->pwm);
  if (err) return err;
  
  err = pd->pwm->ops->stop(pd->pwm);
  if (err) return err;

  return ESUCCESS;
}

static errno_t set_angle(Device_servo *const pd, const angle_t angle) {
  if (pd == NULL) return EINVAL;

  // 周期为 20ms, 其中高电平为 0.5ms 时朝向最左边(-90), 1.5 ms 朝向中间(0), 2.5ms 朝向右边(90)
  const uint32_t pulse = 1500 + angle * 1000 / 90; 
  errno_t err = pd->pwm->ops->set_period(pd->pwm, pulse, 20000);
  if (err) return err;

  pd->angle = angle;

  return ESUCCESS;
}


static errno_t start(Device_servo *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->pwm->ops->start(pd->pwm);
  if (err) return err;

  pd->running = true;

  return ESUCCESS;
}

static errno_t stop(Device_servo *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->pwm->ops->stop(pd->pwm);
  if (err) return err;

  pd->running = false;

  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_servo *)pd)->name == *((Device_servo_name *)name);
}
