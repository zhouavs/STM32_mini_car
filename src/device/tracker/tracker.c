#include "tracker.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

// 对象方法
static errno_t init(Device_tracker *const pd);
static errno_t get_line_center(Device_tracker *const pd, uint8_t *rt_direction_ptr);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_tracker_ops device_ops = {
  .init = init,
  .get_line_center = get_line_center,
};

static List *list = NULL;

errno_t Device_tracker_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_tracker_register(Device_tracker *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_tracker_find(Device_tracker **pd_ptr, const Device_tracker_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(Device_tracker *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  for (uint8_t i = 0; i < DEVICE_TRACKER_IN_COUNT; ++i) {
    err = pd->ins[i]->ops->init(pd->ins[i]);
    if (err) return err;
  }

  // err = pd->timer->ops->init(pd->timer);
  // if (err) return err;

  // // 由于需要微秒级定时, 不能采用滴答定时器
  // if (pd->timer->type == DEVICE_TIMER_TYPE_SYSTICK) {
  //   return EINVAL;
  // }

  // // 10微秒检测一次
  // err = pd->timer->ops->set_period(pd->timer, 10);
  // if (err) return err;

  // // 如果计时器未运行, 启动定时器
  // bool timer_is_running = false;
  // err = pd->timer->ops->is_running(pd->timer, &timer_is_running);
  // if (err) return err;

  // if (timer_is_running == false) {
  //   err = pd->timer->ops->start(pd->timer);
  //   if (err) return err;
  // }

  return ESUCCESS;
}

/**
 * @brief 获取当前轨迹所在位置
 * 
 * @param pd 
 * @param rt_direction_ptr 传返回值的指针, 最高位为是否有轨迹, 后面 7 位表示轨迹中线所在位置, 最左为 0, 最右为 6
 * @return errno_t 
 */
static errno_t get_line_center(Device_tracker *const pd, uint8_t *rt_direction_ptr) {
  if (pd == NULL || rt_direction_ptr == NULL) return EINVAL;

  errno_t err = ESUCCESS;
  // 从 0 到 6 位表示第 0 到 6 个探头的检测结果
  uint8_t ins_value = 0;

  for (uint8_t i = 0; i < DEVICE_TRACKER_IN_COUNT; ++i) {
    uint8_t one_value = 0;
    err = pd->ins[i]->ops->read(pd->ins[i], &one_value);
    if (err) return err;

    if (one_value == PIN_VALUE_1) {
      ins_value |= 1 << i;
    }
  }

  // 如果没有找到轨迹, 返回 0
  if (ins_value == 0) {
    *rt_direction_ptr = 0;
    return ESUCCESS;
  }

  // 中间点为 从左向右最前一个检测到轨迹的点 + (从左向右最后一个检测到轨迹的点 - 从左向右最前一个检测到轨迹的点) / 2
  uint8_t l = 0, r = DEVICE_TRACKER_IN_COUNT - 1;
  while ((ins_value & (1 << l)) == 0) {
    ++l;
  }
  while ((ins_value & (1 << r)) == 0) {
    --r;
  }
  *rt_direction_ptr = (1 << 7) | ((l + (r - l) / 2) & 0x7F);
  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_tracker *)pd)->name == *((Device_tracker_name *)name);
}
