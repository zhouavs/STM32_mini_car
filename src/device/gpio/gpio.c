#include "gpio.h"
#include <stdlib.h>
#include "common/list/list.h"
#include "driver/gpio/gpio.h"

static errno_t init(const Device_GPIO *const pd);
static errno_t read(const Device_GPIO *const pd, Device_GPIO_value *value_ptr);
static errno_t write(const Device_GPIO *const pd, const Device_GPIO_value value);

static inline uint8_t match_device(void *name, void *pd);

static const Device_GPIO_ops device_ops = {
  .init = init,
  .read = read,
  .write = write,
};

static const Driver_GPIO_ops *driver_ops = NULL;
static List *list = NULL;

errno_t Device_GPIO_module_init(void) {
  if (list != NULL) return E_CUSTOM_HAS_INITED;

  errno_t err = Driver_GPIO_get_ops(&driver_ops);
  if (err) return err;

  err = list_create(&list);
  if (err) return err;

  return ESUCCESS;
}

errno_t Device_GPIO_register(Device_GPIO *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, (const void *)pd);
}

errno_t Device_GPIO_find(Device_GPIO **pd_ptr, Device_GPIO_name name) {
  if (list == NULL || name == DEVICE_GPIO_NO_NAME) return EINVAL;

  Device_GPIO *find_pd = NULL;
  errno_t err = list->ops->find(list, &find_pd, &name, match_device);
  if (err) return err;

  *pd_ptr = find_pd;
  return ESUCCESS;
}

static errno_t init(const Device_GPIO *const pd) {
  if (pd == NULL) return EINVAL;

  return ESUCCESS;
}

static errno_t read(const Device_GPIO *const pd, Device_GPIO_value *value_ptr) {
  if (pd == NULL) return EINVAL;
  return driver_ops->read(pd, value_ptr);
}

static errno_t write(const Device_GPIO *const pd, const Device_GPIO_value value) {
  if (pd == NULL) return EINVAL;
  return driver_ops->write(pd, value);
}

static inline uint8_t match_device(void *name, void *pd) {
  return ((Device_GPIO *)pd)->name == *((Device_GPIO_name *)name);
}
