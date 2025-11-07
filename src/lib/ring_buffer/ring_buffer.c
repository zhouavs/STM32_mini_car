#include "ring_buffer.h"
#include <string.h>

static errno_t write(Ring_buffer *prb, uint8_t *data, uint32_t len);
static errno_t read(Ring_buffer *prb, uint8_t *data, uint32_t *data_len, uint32_t len);
static errno_t clear(Ring_buffer *prb);

static const Ring_buffer_ops ops = {
  .write = write,
  .read = read,
  .clear = clear,
};

static errno_t write(Ring_buffer *prb, uint8_t *data, uint32_t len) {
  if (prb == NULL || len == 0) return EINVAL;
  if (len > prb->size - prb->len) return E_CUSTOM_RING_BUFFER_NO_MEMORY;

  uint32_t end = prb->begin + prb->len;
  if (end >= prb->size) end -= prb->size;

  if (prb->begin > end) {
    memcpy(prb->head + end, data, len);
  } else {
    uint32_t end_2_tail_size = prb->size - end;
    if (len <= end_2_tail_size) {
      memcpy(prb->head + end, data, len);
    } else {
      memcpy(prb->head + end, data, end_2_tail_size);
      memcpy(prb->head, data + end_2_tail_size, len - end_2_tail_size);
    }
  }

  prb->len += len;

  return ESUCCESS;
}

static errno_t read(Ring_buffer *prb, uint8_t *data, uint32_t *data_len, uint32_t len) {
  if (prb == NULL) return EINVAL;

  if (len > prb->len) len = prb->len;
  if (len == 0) {
    *data_len = 0;
    return ESUCCESS;
  }

  uint32_t end = prb->begin + prb->len;
  if (end >= prb->size) end -= prb->size;

  if (prb->begin < end) {
    memcpy(data, prb->head + prb->begin, len);
    prb->begin += len;
  } else {
    uint32_t begin_2_tail_size = prb->size - prb->begin;
    if (len <= begin_2_tail_size) {
      memcpy(data, prb->head + prb->begin, len);
      prb->begin += len;
    } else {
      memcpy(data, prb->head + prb->begin, begin_2_tail_size);
      memcpy(data + begin_2_tail_size, prb->head, len - begin_2_tail_size);
      prb->begin = 0 + begin_2_tail_size;
    }
  }

  prb->len -= len;

  return ESUCCESS;
}

static errno_t clear(Ring_buffer *prb) {
  if (prb == NULL) return EINVAL;

  prb->begin = 0;
  prb->len = 0;

  return ESUCCESS;
}

errno_t Ring_buffer_create(Ring_buffer **new_prb_ptr, uint32_t size) {
  Ring_buffer *const prb = (Ring_buffer *)malloc(sizeof(Ring_buffer));
  if (prb == NULL) return ENOMEM;

  prb->head = (uint8_t *)malloc(size);
  if (prb->head == NULL) {
    free(prb);
    return ENOMEM;
  }

  prb->size = size;
  prb->begin = 0;
  prb->len = 0;
  prb->ops = &ops;

  return ESUCCESS;
}

errno_t Ring_buffer_delete(Ring_buffer *del_prb) {
  if (del_prb == NULL) return EINVAL;
  if (del_prb->head != NULL) free(del_prb->head);
  free(del_prb);
  return ESUCCESS;
}
