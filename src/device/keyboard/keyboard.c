#include "keyboard.h"
#include "common/ring_buffer/ring_buffer.h"
#include "common/ring_buffer/ring_buffer.h"

static errno_t read(Device_GPIO_name *key_name);

const static Device_keyboard_ops ops = { .read = read };
const static Device_keyboard device_keyboard = {
  .ops = &ops,
};
static Ring_buffer *ring_buffer = NULL;

errno_t Device_keyboard_module_init(void) {
  if (ring_buffer != NULL) return E_CUSTOM_HAS_INITED;
  errno_t err = Ring_buffer_create(&ring_buffer, 50);
  if (err) return err;
  return ESUCCESS;
}

errno_t Device_keyboard_get_device(const Device_keyboard **pd_ptr) {
  *pd_ptr = &device_keyboard;
  return ESUCCESS;
}

errno_t Device_keyboard_EXTI_callback(const Device_GPIO *const pd) {
  if (pd == NULL || ring_buffer == NULL) return EINVAL;

  Device_GPIO_value value = DEVICE_GPIO_PIN_RESET;
  errno_t err = pd->ops->read(pd, &value);
  if (err) return err;
  if (value == DEVICE_GPIO_PIN_SET) return ESUCCESS;

  err = ring_buffer->ops->write(ring_buffer, (uint8_t *)&pd->name, 1);
  if (err) return err;

  return ESUCCESS;
}

static errno_t read(Device_GPIO_name *key_name) {
  if (ring_buffer == NULL) return EINVAL;

  uint8_t data = 0;
  uint32_t data_len = 0;

  errno_t err = ring_buffer->ops->read(ring_buffer, &data, &data_len, 1);

  if (err) return err;
  if (data_len == 0) return ENODATA;

  *key_name = (Device_GPIO_name)data;

  return ESUCCESS;
}
