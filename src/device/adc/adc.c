#include "adc.h"
#include "common/list/list.h"
#include "driver/adc/adc.h"
#include <stdlib.h>

static errno_t init(Device_ADC *const pd);
static errno_t read(Device_ADC *const pd, uint16_t *rt_data, uint32_t len);

static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_ADC_ops device_ops = {
  .init = init,
  .read = read,
};

static List *list = NULL;
static const Driver_ADC_ops *driver_ops = NULL;

errno_t Device_ADC_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_ADC_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_ADC_register(Device_ADC *const pd) {
  if (list == NULL || pd == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, pd);
}

errno_t Device_ADC_find(Device_ADC **pd_ptr, const Device_ADC_name name) {
  if (list == NULL) return EINVAL;
  return list->ops->find(list, pd_ptr, &name, match_device_by_name);
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return (((Device_ADC *)pd)->name == *(Device_ADC_name *)name);
}

static errno_t init(Device_ADC *const pd) {
  if (pd == NULL) return EINVAL;

  return ESUCCESS;
}

static errno_t read(Device_ADC *const pd, uint16_t *rt_data, uint32_t len) {
  if (pd == NULL || rt_data == NULL || len == 0) return EINVAL;

  errno_t err = ESUCCESS;

  Device_ADC_channel_config config = {
    .channel = pd->channel,
    .sampling_time = pd->sampling_time,
    .rank = 1,
  };
  err = driver_ops->config_channel(pd, &config);
  if (err) return err;

  err = driver_ops->start(pd);
  if (err) return err;

  for (uint32_t i = 0; i < len; ++i) {
    err = driver_ops->poll(pd, 100);
    if (err) return err;
    err = driver_ops->get_value(pd, rt_data + i);
    if (err) return err;
  }

  err = driver_ops->stop(pd);
  if (err) return err;

  return ESUCCESS;
}
