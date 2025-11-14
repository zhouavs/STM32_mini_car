#include "gpio.h"
#include <stdlib.h>
#include "common/list/list.h"
#include "common/ring_buffer/ring_buffer.h"
#include "driver/gpio/gpio.h"

static errno_t init(const Device_GPIO *const pd);
static errno_t read(const Device_GPIO *const pd, Pin_value *value_ptr);
static errno_t write(const Device_GPIO *const pd, const Pin_value value);

static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_GPIO_ops device_ops = {
  .init = init,
  .read = read,
  .write = write,
};

static const Driver_GPIO_ops *driver_ops = NULL;
static List *list = NULL;

errno_t Device_GPIO_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_GPIO_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_GPIO_register(Device_GPIO *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, (const void *)pd);
}

errno_t Device_GPIO_find(Device_GPIO **pd_ptr, Device_GPIO_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

errno_t Device_GPIO_EXTI_callback(Device_GPIO *pd) {
  return ESUCCESS;
}

static errno_t init(const Device_GPIO *const pd) {
  if (pd == NULL) return EINVAL;

  return ESUCCESS;
}

static errno_t read(const Device_GPIO *const pd, Pin_value *value_ptr) {
  if (pd == NULL) return EINVAL;
  return driver_ops->read(pd, value_ptr);
}

static errno_t write(const Device_GPIO *const pd, const Pin_value value) {
  if (pd == NULL) return EINVAL;
  return driver_ops->write(pd, value);
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_GPIO *)pd)->name == *((Device_GPIO_name *)name);
}
