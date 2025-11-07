#include "ring_buffer.h"
#include <string.h>

static errno_t write(Ring_buffer *prb, const uint8_t *data, uint32_t len);
static errno_t read(Ring_buffer *prb, uint8_t *data, uint32_t *data_len, uint32_t len);
static errno_t clear(Ring_buffer *prb);

static inline uint32_t get_ring_buffer_tail(Ring_buffer *prb);

static const Ring_buffer_ops ops = {
  .write = write,
  .read = read,
  .clear = clear,
};

static errno_t write(Ring_buffer *prb, const uint8_t *data, uint32_t len) {
  if (prb == NULL || data == NULL) return EINVAL;
  if (len > prb->size - prb->len) return E_CUSTOM_RING_BUFFER_NO_MEMORY;

  if (len == 0) return ESUCCESS;

  const uint32_t tail = get_ring_buffer_tail(prb);

  if (prb->head > tail) {
    memcpy(prb->data + tail, data, len);
  } else {
    uint32_t tail_free_size = prb->size - tail;

    if (len <= tail_free_size) {
      memcpy(prb->data + tail, data, len);
    } else {
      memcpy(prb->data + tail, data, tail_free_size);
      memcpy(prb->data, data + tail_free_size, len - tail_free_size);
    }
  }

  prb->len += len;

  return ESUCCESS;
}

static errno_t read(Ring_buffer *prb, uint8_t *data, uint32_t *data_len, uint32_t len) {
  if (prb == NULL || data == NULL || data_len == NULL) return EINVAL;

  if (len > prb->len) {
    len = prb->len;
  }
  if (len == 0) {
    *data_len = 0;
    return ESUCCESS;
  }

  const uint32_t tail = get_ring_buffer_tail(prb);

  if (prb->head < tail) {
    memcpy(data, prb->data + prb->head, len);
    prb->head += len;
  } else {
    uint32_t tail_data_size = prb->size - prb->head;

    if (len <= tail_data_size) {
      memcpy(data, prb->data + prb->head, len);
      prb->head += len;
    } else {
      memcpy(data, prb->data + prb->head, tail_data_size);
      memcpy(data + tail_data_size, prb->data, len - tail_data_size);
      prb->head = len - tail_data_size;
    }
  }

  prb->len -= len;
  *data_len = len;

  return ESUCCESS;
}

static errno_t clear(Ring_buffer *prb) {
  if (prb == NULL) return EINVAL;

  prb->head = 0;
  prb->len = 0;

  return ESUCCESS;
}

errno_t Ring_buffer_create(Ring_buffer **new_prb_ptr, uint32_t size) {
  if (new_prb_ptr == NULL || size == 0) return EINVAL;

  Ring_buffer *const prb = (Ring_buffer *)malloc(sizeof(Ring_buffer));
  if (prb == NULL) return ENOMEM;

  prb->data = (uint8_t *)malloc(size);
  if (prb->data == NULL) {
    free(prb);
    return ENOMEM;
  }

  prb->size = size;
  prb->head = 0;
  prb->len = 0;
  prb->ops = &ops;

  *new_prb_ptr = prb;

  return ESUCCESS;
}

errno_t Ring_buffer_delete(Ring_buffer *del_prb) {
  if (del_prb == NULL) return EINVAL;
  if (del_prb->data != NULL) free(del_prb->data);
  free(del_prb);
  return ESUCCESS;
}

static inline uint32_t get_ring_buffer_tail(Ring_buffer *prb) {
  uint32_t end = prb->head + prb->len;
  if (end >= prb->size) end -= prb->size;
  return end;
}
