#include "ring_buffer.h"
#include <string.h>

static errno_t write(Ring_buffer *prb, const uint8_t *data, uint32_t len);
static errno_t read(Ring_buffer *prb, uint8_t *data, uint32_t *data_len, uint32_t len);
static errno_t clear(Ring_buffer *prb);

static inline uint32_t get_ring_buffer_len(uint32_t size, uint32_t write_index, uint32_t read_index);
static inline uint32_t get_new_index(uint32_t size, uint32_t index, uint32_t len);

static const Ring_buffer_ops ops = {
  .write = write,
  .read = read,
  .clear = clear,
};

static errno_t write(Ring_buffer *prb, const uint8_t *data, uint32_t len) {
  if (prb == NULL || data == NULL) return EINVAL;

  // 快照, 避免多次读取可能改变的对象变量
  const uint32_t read_index = prb->read_index;
  uint32_t write_index = prb->write_index;

  const uint32_t data_len = get_ring_buffer_len(prb->size, write_index, read_index);
  if (len > prb->size - data_len) return E_CUSTOM_RING_BUFFER_NO_MEMORY;
  if (len == 0) return ESUCCESS;

  if (read_index > write_index) {
    memcpy(prb->data + write_index, data, len);
    write_index = get_new_index(prb->size, write_index, len);
  } else {
    // data 数组真实长度为 prb->size + 1
    const uint32_t tail_free_len = prb->size + 1 - write_index;

    if (len <= tail_free_len) {
      memcpy(prb->data + write_index, data, len);
      write_index = get_new_index(prb->size, write_index, len);
    } else {
      const uint32_t head_len = len - tail_free_len;
      memcpy(prb->data + write_index, data, tail_free_len);
      memcpy(prb->data, data + tail_free_len, head_len);
      write_index = head_len;
    }
  }

  prb->write_index = write_index;

  return ESUCCESS;
}

static errno_t read(Ring_buffer *prb, uint8_t *rt_data, uint32_t *rt_len, uint32_t len) {
  if (prb == NULL || rt_data == NULL || rt_len == NULL) return EINVAL;
  if (len == 0) {
    *rt_len = 0;
    return ESUCCESS;
  }

  // 快照, 避免多次读取可能改变的对象变量
  uint32_t read_index = prb->read_index;
  const uint32_t write_index = prb->write_index;

  const uint32_t data_len = get_ring_buffer_len(prb->size, write_index, read_index);
  if (len > data_len) len = data_len;

  if (read_index < write_index) {
    memcpy(rt_data, prb->data + read_index, len);
    read_index = get_new_index(prb->size, read_index, len);
  } else {
    // data 数组真实长度为 prb->size + 1
    const uint32_t tail_data_len = prb->size + 1 - read_index;

    if (len <= tail_data_len) {
      memcpy(rt_data, prb->data + read_index, len);
      read_index = get_new_index(prb->size, read_index, len);
    } else {
      const uint32_t head_len = len - tail_data_len;
      memcpy(rt_data, prb->data + read_index, tail_data_len);
      memcpy(rt_data + tail_data_len, prb->data, head_len);
      read_index = head_len;
    }
  }

  prb->read_index = read_index;

  *rt_len = len;

  return ESUCCESS;
}

static errno_t clear(Ring_buffer *prb) {
  if (prb == NULL) return EINVAL;

  prb->read_index = 0;
  prb->write_index = 0;

  return ESUCCESS;
}

errno_t Ring_buffer_create(Ring_buffer **new_prb_ptr, uint32_t size) {
  if (new_prb_ptr == NULL || size == 0) return EINVAL;

  Ring_buffer *const prb = (Ring_buffer *)malloc(sizeof(Ring_buffer));
  if (prb == NULL) return ENOMEM;

  // 空一个字节不写入, 因为在使用 write_index 和 read_index 方案的情况下, 当这两个索引重合的时候无法判断当前缓存为空还是满
  // 只能空一个字节不写入, 这样只有当缓存为空时两个索引才会重合
  prb->data = (uint8_t *)malloc(size + 1);
  if (prb->data == NULL) {
    free(prb);
    return ENOMEM;
  }

  prb->size = size;
  prb->read_index = 0;
  prb->write_index = 0;
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

static inline uint32_t get_ring_buffer_len(uint32_t size, uint32_t write_index, uint32_t read_index) {
  if (write_index >= read_index) return write_index - read_index;
  // 实际长度为 size + 1, 实际长度 - (读下标 - 写下标) 为内容长度
  return size + 1 - (read_index - write_index);
}

static inline uint32_t get_new_index(uint32_t size, uint32_t index, uint32_t len) {
  size++; // data 真实长度
  uint32_t new_index = index + len;
  if (new_index >= size) new_index -= size;
  return new_index;
}
