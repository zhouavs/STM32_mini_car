#include "at24c02.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <string.h>

static errno_t init(const Device_AT24C02 *const pd);
static errno_t read(const Device_AT24C02 *const pd, uint16_t addr, uint8_t *rt_data, uint16_t len);
static errno_t write(const Device_AT24C02 *const pd, uint16_t addr, uint8_t *data, uint16_t len);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_AT24C02_ops device_ops = {
  .init = init,
  .read = read,
  .write = write,
};

static List *list = NULL;

errno_t Device_AT24C02_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_AT24C02_register(Device_AT24C02 *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_AT24C02_find(const Device_AT24C02 **pd_ptr, const Device_AT24C02_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(const Device_AT24C02 *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = pd->i2c->ops->init(pd->i2c);
  if (err) return err;

  return ESUCCESS;
}

static errno_t read(const Device_AT24C02 *const pd, uint16_t addr, uint8_t *rt_data, uint16_t len) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  // at24c 系列大于 2k bit 的产品低 8 位使用 i2c 发送, 高位地址使用引脚控制, 这里暂时只兼容小于等于 2k bit 的产品, 不处理高位地址
  uint8_t lowAddr = (uint8_t)addr;
  err = pd->i2c->ops->transmit(pd->i2c, pd->addr, &lowAddr, 1);
  if (err) return err;

  err = pd->i2c->ops->receive(pd->i2c, pd->addr, rt_data, len);
  if (err) return err;

  return ESUCCESS;
}

static errno_t write(const Device_AT24C02 *const pd, uint16_t addr, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  // 计算从此地址开始的剩余字节数, 如果剩余字节数小于数据长度, 返回参数错误
  if (pd->size - addr < len) return EINVAL;

  errno_t err = ESUCCESS;

  // 先不考虑页大小为 16 字节的情况
  uint8_t send_buf[9] = {0};

  // 本次发送字节数, 初次获取当前页剩余字节数, 后面为页字节数
  const uint8_t first_page_remain = pd->page_size - (addr % pd->page_size);
  uint8_t cur_len = 0;
  if (len > first_page_remain) {
    cur_len = first_page_remain;
    len -= cur_len;
  } else {
    cur_len = len;
    len = 0;
  }

  send_buf[0] = addr;
  memcpy(send_buf + 1, data, cur_len);
  err = pd->i2c->ops->transmit(pd->i2c, pd->addr, send_buf, cur_len + 1);
  if (err) return err;

  // 等待写入完成
  while (pd->i2c->ops->is_device_ready(pd->i2c, pd->addr, 10, 1) != ESUCCESS) {
    delay_ms(1);
  }

  while (len) {
    addr += cur_len;
    data += cur_len;
    if (len > pd->page_size) {
      cur_len = pd->page_size;
      len -= pd->page_size;
    } else {
      cur_len = len;
      len = 0;
    }

    send_buf[0] = addr;
    memcpy(send_buf + 1, data, cur_len);
    err = pd->i2c->ops->transmit(pd->i2c, pd->addr, send_buf, cur_len + 1);
    if (err) {
      return err;
    }

    // 等待写入完成
    while (pd->i2c->ops->is_device_ready(pd->i2c, pd->addr, 10, 1) != ESUCCESS) {
      delay_ms(1);
    }
  }

  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_AT24C02 *)pd)->name == *((Device_AT24C02_name *)name);
}
