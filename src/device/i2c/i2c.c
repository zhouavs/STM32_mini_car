#include "i2c.h"
#include "common/list/list.h"
#include "common/ring_buffer/ring_buffer.h"
#include "driver/i2c/i2c.h"
#include <stdlib.h>

#define MAX_MSG_LEN 0xFFFF

static errno_t init(const Device_I2C *const pd);
static errno_t receive(const Device_I2C *const pd, uint8_t *data, uint32_t len);
static errno_t transmit(const Device_I2C *const pd, uint8_t *const data, uint32_t len);

static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_I2C_ops device_ops = {
  .init = init,
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

static errno_t init(const Device_I2C *const pd) {
  if (pd == NULL) return EINVAL;

  return ESUCCESS;
}

static errno_t transmit(const Device_I2C *const pd, uint8_t *const data, uint32_t len) {
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
      err = driver_ops->master_transmit_IT(pd, data + cur_idx, cur_len);
    } else {
      err = driver_ops->master_transmit_DMA(pd, data + cur_idx, cur_len);
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

static errno_t receive(const Device_I2C *const pd, uint8_t *data, uint32_t len) {
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
      err = driver_ops->master_receive_IT(pd, data + cur_idx, cur_len);
    } else {
      err = driver_ops->master_receive_DMA(pd, data + cur_idx, cur_len);
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
