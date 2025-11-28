#include "ultrasonic.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 计时单位 10 微秒
#define TIME_UNIT_US_NUM 10

// 对象方法
static errno_t init(Device_ultrasonic *const pd);
static errno_t read(Device_ultrasonic *const pd, uint32_t *rt_data_ptr);

// 内部方法
// 等待直到引脚达到某状态
static errno_t wait_until_pin_is(Device_ultrasonic *const pd, const Pin_value target, const uint16_t timeout, uint16_t *rt_time_taken_ptr);
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
  
  err = pd->timer->ops->set_time_for_count_incr(pd->timer, TIME_UNIT_US_NUM);
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

  errno_t err = ESUCCESS;

  err = pd->trig->ops->write(pd->trig, PIN_VALUE_1);
  if (err) return err;
  err = delay_us(15);
  if (err) return err;
  err = pd->trig->ops->write(pd->trig, PIN_VALUE_0);
  if (err) return err;

  err = wait_until_pin_is(pd, PIN_VALUE_1, 3000, NULL);
  if (err) return err;

  uint16_t time_taken = 0;
  err = wait_until_pin_is(pd, PIN_VALUE_0, 5000, &time_taken);
  if (err) return err;

  // 距离单位为 0.1 毫米, 计算公式为: 声速 / 2 * 秒数 * 10000
  // 即: 340 / 2 * (ime_taken / (1000000 / TIME_UNIT_US_NUM)) * 10000, 化简如下
  uint32_t distance = 34 / 2 * time_taken * TIME_UNIT_US_NUM / 10;

  *rt_data_ptr = distance;

  return ESUCCESS;
}

/**
 * @brief 等待引脚到达指定状态
 * 
 * @param pd 设备对象
 * @param target 需要到达的状态
 * @param timeout 超时时长
 * @param rt_time_taken_ptr 接收返回的实际用时时长, 传 NULL 不返回
 * @return errno_t 返回的错误码
 */
static errno_t wait_until_pin_is(Device_ultrasonic *const pd, const Pin_value target, const uint16_t timeout, uint16_t *rt_time_taken_ptr) {
  uint32_t first_tick32 = 0;
  errno_t err = pd->timer->ops->get_register_count(pd->timer, &first_tick32);
  if (err) return err;
  uint16_t first_tick = (uint16_t)first_tick32;

  Pin_value pv = PIN_VALUE_1;
  uint16_t tick = first_tick;

  for (;;) {
    err = pd->echo->ops->read(pd->echo, &pv);
    if (err) return err;

    uint32_t tick32 = 0;
    err = pd->timer->ops->get_register_count(pd->timer, &tick32);
    if (err) return err;
    tick = (uint16_t)tick32;

    uint16_t diff = (uint16_t)(tick - first_tick);

    if (pv == target) {
      if (rt_time_taken_ptr != NULL) {
        *rt_time_taken_ptr = diff;
      }
      return ESUCCESS;
    }

    if (diff >= timeout) break;
  }

  return ETIMEDOUT;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_ultrasonic *)pd)->name == *((Device_ultrasonic_name *)name);
}
