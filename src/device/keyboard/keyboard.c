#include "keyboard.h"
#include "common/list/list.h"
#include "common/ring_buffer/ring_buffer.h"

static errno_t read(Device_key_name *key_name);
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

const static Device_keyboard_ops device_ops = { .read = read };
static Ring_buffer *ring_buffer = NULL;
static List *list = NULL;

errno_t Device_keyboard_module_init(void) {
  if (ring_buffer == NULL) {
    errno_t err = Ring_buffer_create(&ring_buffer, 50);
    if (err) return err;
  }
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }
  return ESUCCESS;
}

errno_t Device_keyboard_register(Device_keyboard *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, (const void *)pd);
}

errno_t Device_keyboard_find(const Device_keyboard **pd_ptr, Device_keyboard_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

errno_t Device_keyboard_in_EXTI_callback(const Device_keyboard *const pd, const Device_key_name key) {
  if (pd == NULL || ring_buffer == NULL) return EINVAL;

  Device_GPIO *const in = pd->ins[key];
  Pin_value value = PIN_VALUE_0;
  errno_t err = in->ops->read(in, &value);
  if (err) return err;
  if (value == PIN_VALUE_1) return ESUCCESS;

  err = ring_buffer->ops->write(ring_buffer, (uint8_t *)&key, 1);
  if (err) return err;

  return ESUCCESS;
}

static errno_t read(Device_key_name *key_name) {
  if (ring_buffer == NULL) return EINVAL;

  uint8_t data = 0;
  uint32_t data_len = 0;

  errno_t err = ring_buffer->ops->read(ring_buffer, &data, &data_len, 1);

  if (err) return err;
  if (data_len == 0) return ENODATA;

  *key_name = (Device_key_name)data;

  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_keyboard *)pd)->name == *((Device_keyboard_name *)name);
}
