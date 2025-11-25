#include "motor.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 对象方法
static errno_t init(Device_motor *const pd);
static errno_t stop(Device_motor *const pd);
static errno_t forward(Device_motor *const pd, const speed_t speed);
static errno_t backward(Device_motor *const pd, const speed_t speed);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_motor_ops device_ops = {
  .init = init,
  .stop = stop,
  .forward = forward,
  .backward = backward,
};

static List *list = NULL;

errno_t Device_motor_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_motor_register(Device_motor *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_motor_find(Device_motor **pd_ptr, const Device_motor_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(Device_motor *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->pwm->ops->init(pd->pwm);
  if (err) return err;
  err = pd->in_1->ops->init(pd->in_1);
  if (err) return err;
  err = pd->in_2->ops->init(pd->in_2);
  if (err) return err;
  
  err = pd->in_1->ops->write(pd->in_1, PIN_VALUE_0);
  if (err) return err;
  err = pd->in_2->ops->write(pd->in_2, PIN_VALUE_0);
  if (err) return err;
  err = pd->pwm->ops->stop(pd->pwm);
  if (err) return err;

  return ESUCCESS;
}

static errno_t stop(Device_motor *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->in_1->ops->write(pd->in_1, PIN_VALUE_0);
  if (err) return err;
  err = pd->in_2->ops->write(pd->in_2, PIN_VALUE_0);
  if (err) return err;
  err = pd->pwm->ops->stop(pd->pwm);
  if (err) return err;

  pd->status = DEVICE_MOTOR_STATUS_STOP;

  return ESUCCESS;
}

static errno_t forward(Device_motor *const pd, const speed_t speed) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->pwm->ops->set_period(pd->pwm, speed, 0xFF);
  if (err) return err;
  err = pd->in_1->ops->write(pd->in_1, PIN_VALUE_1);
  if (err) return err;
  err = pd->in_2->ops->write(pd->in_2, PIN_VALUE_0);
  if (err) return err;

  bool running = false;
  err = pd->pwm->ops->is_running(pd->pwm, &running);
  if (err) return err;
  if (running == false) {
    err = pd->pwm->ops->start(pd->pwm);
    if (err) return err;
  }

  pd->speed = speed;
  pd->status = DEVICE_MOTOR_STATUS_FORWARD;

  return ESUCCESS;
}

static errno_t backward(Device_motor *const pd, const speed_t speed) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->pwm->ops->set_period(pd->pwm, speed, 0xFF);
  if (err) return err;
  err = pd->in_1->ops->write(pd->in_1, PIN_VALUE_0);
  if (err) return err;
  err = pd->in_2->ops->write(pd->in_2, PIN_VALUE_1);
  if (err) return err;

  bool running = false;
  err = pd->pwm->ops->is_running(pd->pwm, &running);
  if (err) return err;
  if (running == false) {
    err = pd->pwm->ops->start(pd->pwm);
    if (err) return err;
  }
  
  pd->speed = speed;
  pd->status = DEVICE_MOTOR_STATUS_BACKWARD;

  return ESUCCESS;
}


static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_motor *)pd)->name == *((Device_motor_name *)name);
}
