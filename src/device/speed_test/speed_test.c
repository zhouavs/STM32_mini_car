#include "speed_test.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

// 对象方法
static errno_t init(Device_speed_test *const pd);
static errno_t get_speed(Device_speed_test *const pd, float *rt_speed_ptr);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_speed_test_ops device_ops = {
  .init = init,
  .get_speed = get_speed,
};

static List *list = NULL;
static volatile uint32_t counts[DEVICE_SPEED_TEST_COUNT] = {0};
static uint32_t last_read_ticks[DEVICE_SPEED_TEST_COUNT] = {0};

errno_t Device_speed_test_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_speed_test_register(Device_speed_test *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_speed_test_find(Device_speed_test **pd_ptr, const Device_speed_test_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

errno_t Device_speed_test_EXTI_callback(Device_speed_test *pd) {
  if (pd == NULL) return EINVAL;

  ++counts[pd->name];

  return ESUCCESS;
}

static errno_t init(Device_speed_test *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->in->ops->init(pd->in);
  if (err) return err;

  err = pd->timer->ops->init(pd->timer);
  if (err) return err;

  // 如果采用通用计时器, 那么设定事件单位为 1 毫秒, 因为滴答定时器默认为 1 毫秒且无法更改, 此处保持一致
  if (pd->timer->type == DEVICE_TIMER_TYPE_GENERAL) {
    err = pd->timer->ops->set_period(pd->timer, 1000);
    if (err) return err;
  }

  // 如果计时器未运行, 启动定时器
  bool timer_is_running = false;
  err = pd->timer->ops->is_running(pd->timer, &timer_is_running);
  if (err) return err;

  if (timer_is_running == false) {
    err = pd->timer->ops->start(pd->timer, DEVICE_TIMER_START_MODE_IT);
    if (err) return err;
  }

  counts[pd->name] = 0;

  return ESUCCESS;
}

static errno_t get_speed(Device_speed_test *const pd, float *rt_speed_ptr) {
  if (pd == NULL || rt_speed_ptr == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  uint32_t cur_read_tick = 0;
  err = pd->timer->ops->get_count(pd->timer, &cur_read_tick);
  if (err) return err;

  // 经过的毫秒数
  uint32_t tick_num = cur_read_tick - last_read_ticks[pd->name];
  uint32_t count = counts[pd->name];

  last_read_ticks[pd->name] = cur_read_tick;
  counts[pd->name] = 0;

  // 要返回马达每秒转动的圈数, 其中每40个count对应一圈
  *rt_speed_ptr = (float)count * 1000 / 40 / tick_num;

  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_speed_test *)pd)->name == *((Device_speed_test_name *)name);
}
