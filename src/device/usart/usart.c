#include "usart.h"
#include "common/list/list.h"
#include "common/ring_buffer/ring_buffer.h"
#include "driver/usart/usart.h"
#include <stdlib.h>

static errno_t init(Device_USART *pd);
static errno_t receive(Device_USART *pd, uint8_t *data, uint32_t *data_len, uint32_t len);
static errno_t transmit(Device_USART *pd, uint8_t *data, uint32_t len);

static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_USART_ops device_ops = {
  .init = init,
  .receive = receive,
  .transmit = transmit,
};

static List *list = NULL;
static const Driver_USART_ops *driver_ops = NULL;
static Ring_buffer *ring_buffers[DEVICE_USART_COUNT] = {0};
static volatile uint8_t rx_bytes[DEVICE_USART_COUNT] = {0};
static volatile uint8_t transmitting[DEVICE_USART_COUNT] = {0};

errno_t Device_USART_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_USART_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_USART_register(Device_USART *const pd) {
  if (list == NULL || pd == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, pd);
}

errno_t Device_USART_find(Device_USART **pd_ptr, const Device_USART_name name) {
  if (list == NULL) return EINVAL;
  return list->ops->find(list, pd_ptr, &name, match_device_by_name);
}

errno_t Device_USART_TxCpltCallback(Device_USART *pd) {
  transmitting[pd->name] = 0;
  return ESUCCESS;
}

errno_t Device_USART_RxCpltCallback(Device_USART *pd) {
  Ring_buffer *prb = ring_buffers[pd->name];
  errno_t err = prb->ops->write(prb, (uint8_t *)&rx_bytes[pd->name], 1);
  if (err) return err;
  err = driver_ops->receive_IT(pd, (uint8_t *)&rx_bytes[pd->name], 1);
  if (err) return err;
  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return (((Device_USART *)pd)->name == *(Device_USART_name *)name);
}

static errno_t init(Device_USART *pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  if (ring_buffers[pd->name] == NULL) {
    Ring_buffer *rb = NULL;
    err = Ring_buffer_create(&rb, 255);
    if (err) return err;
    ring_buffers[pd->name] = rb;
  }

  err = driver_ops->abort_receive(pd);
  if (err) return err;
  err = driver_ops->receive_IT(pd, (uint8_t *)&rx_bytes[pd->name], 1);
  if (err) return err;

  return ESUCCESS;
}

static errno_t transmit(Device_USART *pd, uint8_t *data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  errno_t err = ESUCCESS;
  transmitting[pd->name] = 1;
  err = driver_ops->transmit_IT(pd, data, len);
  if (err) return err;
  while (transmitting[pd->name]);
  return ESUCCESS;
}

static errno_t receive(Device_USART *pd, uint8_t *data, uint32_t *data_len, uint32_t len) {
  if (pd == NULL || data == NULL || data_len == NULL || len == 0) return EINVAL;
  Ring_buffer *rb = ring_buffers[pd->name];
  if (rb == NULL) return EINVAL;
  return rb->ops->read(rb, data, data_len, len);
}
