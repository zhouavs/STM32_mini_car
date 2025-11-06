#include "gpio.h"
#include <stdlib.h>
#include "common/list/list.h"

static List *list = NULL;

static uint8_t _match_device(void *name, void *pd) {
  return ((Device_GPIO *)pd)->name == *((Device_GPIO_name *)name);
}

errno_t Device_GPIO_module_init(void) {
  if (list != NULL) return E_CUSTOM_HAS_INITED;
  return list_create(&list);
}

errno_t Device_GPIO_register(const Device_GPIO *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  return list->ops->head_insert(list, pd);
}

errno_t Device_GPIO_find(Device_GPIO *pd, Device_GPIO_name name) {
  if (list == NULL || name == DEVICE_GPIO_NO_NAME) return EINVAL;
  return list->ops->find(list, &pd, &name, _match_device);
}
