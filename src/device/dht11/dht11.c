#include "dht11.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NUM_IS_VALUE(num, value, margin) ((uint32_t)num >= (uint32_t)value - (uint32_t)margin && (uint32_t)num <= (uint32_t)value + (uint32_t)margin)


// 对象方法
static errno_t init(Device_DHT11 *const pd);
static errno_t read(Device_DHT11 *const pd, uint8_t rt_data[4]);

// 内部方法
// 等待引脚到达某个值
static errno_t wait_until_pin_is(Device_DHT11 *const pd, const Pin_value target, const uint16_t timeout_us, uint16_t *rt_time_taken_ptr);
// 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_DHT11_ops device_ops = {
  .init = init,
  .read = read,
};

static List *list = NULL;

errno_t Device_DHT11_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_DHT11_register(Device_DHT11 *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_DHT11_find(Device_DHT11 **pd_ptr, const Device_DHT11_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(Device_DHT11 *const pd) {
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

  err = pd->in->ops->init(pd->in);
  if (err) return err;

  return ESUCCESS;
}

static errno_t read(Device_DHT11 *const pd, uint8_t rt_data[4]) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->in->ops->write(pd->in, PIN_VALUE_0);
  if (err) return err;
  err = delay_ms(20);
  if (err) return err;
  err = pd->in->ops->write(pd->in, PIN_VALUE_1);
  if (err) return err;

  err = wait_until_pin_is(pd, PIN_VALUE_0, 100, NULL);
  if (err) return err;
  err = wait_until_pin_is(pd, PIN_VALUE_1, 100, NULL);
  if (err) return err;
  err = wait_until_pin_is(pd, PIN_VALUE_0, 100, NULL);
  if (err) return err;

  // 一次完整的数据传输为40bit, 高位先出。
  // 数据格式:
  // 8bit湿度整数数据 + 8bit湿度小数数据 + 8bit温度整数数据 + 8bit温度小数数据 + 8bit校验和
  uint8_t bytes[5] = {0};

  for (uint8_t byte_idx = 0; byte_idx < 5; byte_idx++) {
    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
      err = wait_until_pin_is(pd, PIN_VALUE_1, 100, NULL);
      if (err) 
        return err;

      uint16_t time_taken = 0;

      err = wait_until_pin_is(pd, PIN_VALUE_0, 100, &time_taken);
      if (err)
        return err;

      if (time_taken < 50) {
        // 高电平时长为 26 - 28 微秒时表示 0
      } else {
        // 高电平时长为 70 微秒时表示 1, 先发高位
        bytes[byte_idx] |= 1 << (7 - bit_idx);
      }
    }
  }

  // 等待设备释放总线
  err = wait_until_pin_is(pd, PIN_VALUE_1, 60, NULL);
  if (err)
    return err;

  // 校验和数据应等于 8bit湿度整数数据 + 8bit湿度小数数据 + 8bit温度整数数据 + 8bit温度小数数据 所得结果的末8位
  uint8_t sum = (uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
  if (sum != bytes[4])
    return EIO;

  for (uint8_t i = 0; i < 4; i++) {
    rt_data[i] = bytes[i];
  }

  return ESUCCESS;
}

/**
 * @brief 等待引脚到达指定状态
 * 
 * @param pd 设备对象
 * @param target 需要到达的状态
 * @param timeout_us 超时时长
 * @param rt_time_taken_ptr 接收返回的实际用时时长, 传 NULL 不返回
 * @return errno_t 返回的错误码
 */
static errno_t wait_until_pin_is(Device_DHT11 *const pd, const Pin_value target, const uint16_t timeout_us, uint16_t *rt_time_taken_ptr) {
  uint32_t first_tick32 = 0;
  errno_t err = pd->timer->ops->get_register_count(pd->timer, &first_tick32);
  if (err) return err;
  uint16_t first_tick = (uint16_t)first_tick32;

  Pin_value pv = PIN_VALUE_1;
  uint16_t tick = first_tick;

  for (;;) {
    err = pd->in->ops->read(pd->in, &pv);
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

    if (diff >= timeout_us) break;
  }

  return ETIMEDOUT;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_DHT11 *)pd)->name == *((Device_DHT11_name *)name);
}
