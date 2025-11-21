#include "i2c.h"
#include "common/list/list.h"
#include "common/ring_buffer/ring_buffer.h"
#include "driver/i2c/i2c.h"
#include <stdlib.h>

#define MAX_MSG_LEN 0xFFFF

static errno_t init(Device_I2C *const pd);
static errno_t is_device_ready(const Device_I2C *const pd, uint16_t slave_addr, uint32_t trial_num, uint32_t timeout);
static errno_t receive(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *data, uint32_t len);
static errno_t transmit(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *const data, uint32_t len);

static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_I2C_ops device_ops = {
  .init = init,
  .is_device_ready = is_device_ready,
  .receive = receive,
  .transmit = transmit,
};

static List *list = NULL;
static const Driver_I2C_ops *driver_ops = NULL;
static volatile uint8_t receiving[DEVICE_I2C_COUNT] = {0};
static volatile uint8_t transmitting[DEVICE_I2C_COUNT] = {0};

errno_t Device_I2C_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_I2C_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_I2C_register(Device_I2C *const pd) {
  if (list == NULL || pd == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, pd);
}

errno_t Device_I2C_find(Device_I2C **pd_ptr, const Device_I2C_name name) {
  if (list == NULL) return EINVAL;
  return list->ops->find(list, pd_ptr, &name, match_device_by_name);
}

errno_t Device_I2C_TxCpltCallback(const Device_I2C *const pd) {
  transmitting[pd->name] = 0;
  return ESUCCESS;
}

errno_t Device_I2C_RxCpltCallback(const Device_I2C *const pd) {
  receiving[pd->name] = 0;
  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return (((Device_I2C *)pd)->name == *(Device_I2C_name *)name);
}

static errno_t init(Device_I2C *const pd) {
  if (pd == NULL) return EINVAL;

  return driver_ops->get_own_addr(pd, &pd->own_addr);

  return ESUCCESS;
}

static errno_t is_device_ready(const Device_I2C *const pd, uint16_t slave_addr, uint32_t trial_num, uint32_t timeout) {
  if (pd == NULL) return EINVAL;

  errno_t err = driver_ops->is_device_ready(pd, slave_addr, trial_num, timeout);
  if (err) return err;

  return ESUCCESS;
}

static errno_t transmit(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *const data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;

  uint32_t cur_idx = 0;
  uint32_t cur_len = 0;

  do {
    if (len > MAX_MSG_LEN) {
      cur_len = MAX_MSG_LEN;
      len -= MAX_MSG_LEN;
    } else {
      cur_len = len;
      len = 0;
    }

    transmitting[pd->name] = 1;
    errno_t err = ESUCCESS;
    if (cur_len == 1) {
      err = driver_ops->master_transmit_IT(pd, slave_addr, data + cur_idx, cur_len);
    } else {
      err = driver_ops->master_transmit_DMA(pd, slave_addr, data + cur_idx, cur_len);
    }
    if (err) {
      transmitting[pd->name] = 0;
      return err;
    }
    while (transmitting[pd->name]);

    cur_idx += cur_len;
  } while (len > 0);

  return ESUCCESS;
}

static errno_t receive(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;

  uint32_t cur_idx = 0;
  uint32_t cur_len = 0;

  do {
    if (len > MAX_MSG_LEN) {
      cur_len = MAX_MSG_LEN;
      len -= MAX_MSG_LEN;
    } else {
      cur_len = len;
      len = 0;
    }

    receiving[pd->name] = 1;
    errno_t err = ESUCCESS;
    if (cur_len == 1) {
      err = driver_ops->master_receive_IT(pd, slave_addr, data + cur_idx, cur_len);
    } else {
      err = driver_ops->master_receive_DMA(pd, slave_addr, data + cur_idx, cur_len);
    }
    if (err) {
      receiving[pd->name] = 0;
      return err;
    }
    while (receiving[pd->name]);

    cur_idx += cur_len;
  } while (len > 0);

  return ESUCCESS;
}
