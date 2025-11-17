#include "spi.h"
#include "common/list/list.h"
#include "common/ring_buffer/ring_buffer.h"
#include "driver/spi/spi.h"
#include <stdlib.h>

#define MAX_MSG_LEN 0xFFFF

static errno_t init(const Device_SPI *const pd);
static errno_t receive(const Device_SPI *const pd, uint8_t *data, uint16_t len);
static errno_t transmit(const Device_SPI *const pd, const uint8_t *const data, uint16_t len);

static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_SPI_ops device_ops = {
  .init = init,
  .receive = receive,
  .transmit = transmit,
};

static List *list = NULL;
static const Driver_SPI_ops *driver_ops = NULL;
static volatile uint8_t receiving[DEVICE_SPI_COUNT] = {0};
static volatile uint8_t transmitting[DEVICE_SPI_COUNT] = {0};

errno_t Device_SPI_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_SPI_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_SPI_register(Device_SPI *const pd) {
  if (list == NULL || pd == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, pd);
}

errno_t Device_SPI_find(Device_SPI **pd_ptr, const Device_SPI_name name) {
  if (list == NULL) return EINVAL;
  return list->ops->find(list, pd_ptr, &name, match_device_by_name);
}

errno_t Device_SPI_TxCpltCallback(const Device_SPI *const pd) {
  transmitting[pd->name] = 0;
  return ESUCCESS;
}

errno_t Device_SPI_RxCpltCallback(const Device_SPI *const pd) {
  receiving[pd->name] = 0;
  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return (((Device_SPI *)pd)->name == *(Device_SPI_name *)name);
}

static errno_t init(const Device_SPI *const pd) {
  if (pd == NULL) return EINVAL;

  return ESUCCESS;
}

static errno_t transmit(const Device_SPI *const pd, const uint8_t *const data, uint16_t len) {
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
    driver_ops->transmit_DMA(pd, data + cur_idx, cur_len);
    cur_idx += cur_len;

    while (transmitting[pd->name]);
  } while (len > 0);

  return ESUCCESS;
}

static errno_t receive(const Device_SPI *const pd, uint8_t *data, uint16_t len) {
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
    driver_ops->receive_DMA(pd, data + cur_idx, cur_len);
    cur_idx += cur_len;

    while (receiving[pd->name]);
  } while (len > 0);

  return ESUCCESS;
}
