#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "common/errno/errno.h"

struct Ring_buffer_ops;

typedef struct Ring_buffer {
  uint8_t *head;
  uint32_t size;
  uint32_t begin;
  uint32_t len;
  const struct Ring_buffer_ops *ops;
} Ring_buffer;

typedef struct Ring_buffer_ops {
  errno_t (*write)(Ring_buffer *prb, uint8_t *data, uint32_t len);
  errno_t (*read)(Ring_buffer *prb, uint8_t *data, uint32_t *data_len, uint32_t len);
  errno_t (*clear)(Ring_buffer *prb);
} Ring_buffer_ops;

errno_t Ring_buffer_create(Ring_buffer **new_prb_ptr, uint32_t size);
errno_t Ring_buffer_delete(Ring_buffer *del_prb);
