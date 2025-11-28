#include "irda.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include "common/ring_buffer/ring_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NUM_IS_VALUE(num, value, margin) ((uint32_t)num >= (uint32_t)value - (uint32_t)margin && (uint32_t)num <= (uint32_t)value + (uint32_t)margin)

typedef enum {
  STATE_IDLE,
  STATE_LEADER_FIRST,
  STATE_LEADER_SECOND,
  STATE_DATA,
} IRDA_state;

typedef enum {
  CODE_LEADER_FIRST = 900,
  CODE_LEADER_SECOND = 450,
  CODE_LEADER_REPEAT_SECOND = 225,
  CODE_DATA_FIRST = 56,
  CODE_DATA_0_SECOND = 56,
  CODE_DATA_1_SECOND = 169
} IRDA_code;

// 对象方法
static errno_t init(Device_IRDA *const pd);
static errno_t read(Device_IRDA *const pd, Device_IRDA_cmd *rt_cmd_ptr);

// 内部方法
// 解码
static errno_t decode(Device_IRDA *const pd, Device_IRDA_cmd *rt_cmd_ptr);
// 读取 tick
static errno_t read_tick(Device_IRDA *const pd, uint32_t *rt_tick_ptr, uint32_t timeout);
// 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_IRDA_ops device_ops = {
  .init = init,
  .read = read,
};

static List *list = NULL;
static Ring_buffer *ring_buffers[DEVICE_IRDA_COUNT] = {0};
static Device_IRDA_cmd last_cmds[DEVICE_IRDA_COUNT] = {0};

errno_t Device_IRDA_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_IRDA_register(Device_IRDA *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_IRDA_find(Device_IRDA **pd_ptr, const Device_IRDA_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

// 外部中断回调
errno_t Device_IRDA_in_EXTI_callback(Device_IRDA *const pd) {
  if (pd == NULL) return EINVAL;

  Ring_buffer *const rb = ring_buffers[pd->name];
  if (rb == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  uint32_t tick = 0;
  err = pd->timer->ops->get_count(pd->timer, &tick);
  if (err) return err;

  // 读出的时候也是直接读出4位然后转换指针, 所以这里不用特殊处理大小端
  err = rb->ops->write(rb, (uint8_t *)&tick, sizeof(uint32_t));
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(Device_IRDA *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->timer->ops->init(pd->timer);
  if (err) return err;
  
  // 计时单位为10微秒, 因为 irda 信号的最小单位为 10 微秒
  err = pd->timer->ops->set_period(pd->timer, 10);
  if (err) return err;
  
  err = pd->timer->ops->start(pd->timer, DEVICE_TIMER_START_MODE_IT);
  if (err) return err;

  if (ring_buffers[pd->name] == NULL) {
    err = Ring_buffer_create(&ring_buffers[pd->name], sizeof(uint32_t) * 0x100);
    if (err) return err;
  }

  err = pd->in->ops->init(pd->in);
  if (err) return err;

  return ESUCCESS;
}

static errno_t read(Device_IRDA *const pd, Device_IRDA_cmd *rt_cmd_ptr) {
  if (pd == NULL || rt_cmd_ptr == NULL) return EINVAL;
  return decode(pd, rt_cmd_ptr);
}

static errno_t decode(Device_IRDA *const pd, Device_IRDA_cmd *rt_cmd_ptr) {
  if (pd == NULL || rt_cmd_ptr == NULL) return EINVAL;
  Ring_buffer *rb = ring_buffers[pd->name];
  if (rb == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  IRDA_state state = STATE_IDLE;
  const uint32_t timeout = 1000;
  uint32_t cur_tick = 0;
  uint32_t last_tick = 0;

  while (true) {
    switch (state) {
      case STATE_IDLE: {
        err = read_tick(pd, &cur_tick, timeout);
        if (err) return err;

        last_tick = cur_tick;
        state = STATE_LEADER_FIRST;

        break;
      }
      case STATE_LEADER_FIRST: {
        err = read_tick(pd, &cur_tick, timeout);
        if (err) return err;

        const uint32_t diff = cur_tick - last_tick;
        last_tick = cur_tick;

        if (NUM_IS_VALUE(diff, CODE_LEADER_FIRST, CODE_LEADER_FIRST / 10)) {
          state = STATE_LEADER_SECOND;
        } else {
          state = STATE_IDLE;
        }

        break;
      }
      case STATE_LEADER_SECOND: {
        err = read_tick(pd, &cur_tick, timeout);
        if (err) return err;

        const uint32_t diff = cur_tick - last_tick;
        last_tick = cur_tick;

        if (NUM_IS_VALUE(diff, CODE_LEADER_SECOND, CODE_LEADER_SECOND / 10)) {
          state = STATE_DATA;
        } else if (NUM_IS_VALUE(diff, CODE_LEADER_REPEAT_SECOND, CODE_LEADER_REPEAT_SECOND / 10)) {
          // 引导码(重复)获取上次命令
          *rt_cmd_ptr = last_cmds[pd->name];
          return ESUCCESS;
        } else {
          state = STATE_IDLE;
        }
        
        break;
      }
      case STATE_DATA: {
        // 每次发送 4 个字节数据, 分别是 地址 -> 地址反码 -> 命令 -> 命令反码
        uint8_t bytes[4] = {0};

        for (uint8_t byte_idx = 0; byte_idx < 4; byte_idx++) {
          for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            // 每一个数据位的前半部分和后半部分电平持续事件
            uint32_t diffs[2] = {0};

            for (uint8_t i = 0; i < 2; i++) {
              err = read_tick(pd, &cur_tick, timeout);
              if (err) return err;
  
              diffs[i] = cur_tick - last_tick;
              last_tick = cur_tick;
            }

            if (NUM_IS_VALUE(diffs[0], CODE_DATA_FIRST, CODE_DATA_FIRST / 5)) {
              if (NUM_IS_VALUE(diffs[1], CODE_DATA_0_SECOND, CODE_DATA_0_SECOND / 5)) {
                // 该位为 0
              } else if (NUM_IS_VALUE(diffs[1], CODE_DATA_1_SECOND, CODE_DATA_1_SECOND / 5)) {
                // 该位为 1
                bytes[byte_idx] |= (1 << bit_idx);
              } else {
                return EIO;
              }
            } else {
              return EIO;
            }
          }
        }

        // 校验反码, 对 uint8_t 变量进行取反操作时, 会自动提升原变量为整型然后再进行取反, 所以需要强制转换一下取反后的类型
        if (bytes[0] != (uint8_t)~bytes[1]) return EIO;
        if (bytes[2] != (uint8_t)~bytes[3]) return EIO;

        *rt_cmd_ptr = bytes[2];

        last_cmds[pd->name] = *rt_cmd_ptr;

        return ESUCCESS;
      }
      default: {
        return EINVAL;
      }
    }
  }
}

static errno_t read_tick(Device_IRDA *const pd, uint32_t *rt_tick_ptr, uint32_t timeout) {
  Ring_buffer *rb = ring_buffers[pd->name];
  errno_t err = ESUCCESS;
  uint32_t cur_tick = 0, read_len = 0;

  while (timeout--) {
    err = rb->ops->read(rb, (uint8_t *)&cur_tick, &read_len, sizeof(uint32_t));

    if (err) return err;
    if (read_len == sizeof(uint32_t)) {
      *rt_tick_ptr = cur_tick;
      return ESUCCESS;
    }

    delay_ms(1);
  }

  return ETIMEDOUT;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_IRDA *)pd)->name == *((Device_IRDA_name *)name);
}
