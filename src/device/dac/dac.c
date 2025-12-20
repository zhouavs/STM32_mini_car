#include "dac.h"
#include "common/list/list.h"
#include "driver/dac/dac.h"
#include <stdlib.h>

static errno_t init(Device_DAC *const pd);
static errno_t reset(Device_DAC *const pd);
static errno_t set_point(Device_DAC *const pd, uint16_t point);
static errno_t set_wave(Device_DAC *const pd, uint16_t *points, uint16_t len, uint16_t refresh_interval_us);

static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_DAC_ops device_ops = {
  .init = init,
  .reset = reset,
  .set_point = set_point,
  .set_wave = set_wave,
};

static List *list = NULL;
static const Driver_DAC_ops *driver_ops = NULL;

errno_t Device_DAC_module_init(void) {
  if (driver_ops == NULL) {
    errno_t err = Driver_DAC_get_ops(&driver_ops);
    if (err) return err;
  }

  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_DAC_register(Device_DAC *const pd) {
  if (list == NULL || pd == NULL) return EINVAL;
  pd->ops = &device_ops;
  return list->ops->head_insert(list, pd);
}

errno_t Device_DAC_find(Device_DAC **pd_ptr, const Device_DAC_name name) {
  if (list == NULL) return EINVAL;
  return list->ops->find(list, pd_ptr, &name, match_device_by_name);
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return (((Device_DAC *)pd)->name == *(Device_DAC_name *)name);
}

static errno_t init(Device_DAC *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  if (pd->timer != NULL) {
    err = pd->timer->ops->init(pd->timer);
    if (err) return err;
  }

  return ESUCCESS;
}

static errno_t reset(Device_DAC *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  switch (pd->run_status) {
    case DEVICE_DAC_RUN_STATUS_NONE: {
      return ESUCCESS;
    }
    case DEVICE_DAC_RUN_STATUS_POINT: {
      err = driver_ops->stop(pd);
      if (err) return err;
      pd->run_status = DEVICE_DAC_RUN_STATUS_NONE;
      return ESUCCESS;
    }
    case DEVICE_DAC_RUN_STATUS_WAVE: {
      err = pd->timer->ops->stop(pd->timer);
      if (err) return err;
      err = driver_ops->stop_DMA(pd);
      if (err) return err;
      pd->run_status = DEVICE_DAC_RUN_STATUS_NONE;
      return ESUCCESS;
    }
    default: {
      return EINVAL;
    }
  }
  // Fallback: should not reach here
  return ESUCCESS;
}

static errno_t set_point(Device_DAC *const pd, uint16_t point) {
  if (pd == NULL) return EINVAL;
  if (pd->run_status != DEVICE_DAC_RUN_STATUS_NONE) return EBUSY;

  errno_t err = ESUCCESS;

  err = driver_ops->set_value(pd, point, DEVICE_DAC_ALIGN_12B_R);
  if (err) return err;

  err = driver_ops->start(pd);
  if (err) return err;

  pd->run_status = DEVICE_DAC_RUN_STATUS_POINT;

  return ESUCCESS;
}

static errno_t set_wave(Device_DAC *const pd, uint16_t *points, uint16_t len, uint16_t refresh_interval_us) {
  if (pd == NULL) return EINVAL;
  if (pd->timer == NULL) return EINVAL;
  if (pd->run_status != DEVICE_DAC_RUN_STATUS_NONE) return EBUSY;

  Device_DAC_channel_config config = {
    .output_buffer = DEVICE_DAC_OUTPUT_BUFFER_ENABLE,
    .trigger = DEVICE_DAC_TRIGGER_NONE,
  };
  
  switch (pd->timer->name) {
    case DEVICE_TIMER_TIM4: {
      config.trigger = DEVICE_DAC_TRIGGER_T4_TRGO;
      break;
    }
    default: {
      return EINVAL;
    }
  }

  errno_t err = ESUCCESS;

  err = pd->timer->ops->set_period(pd->timer, refresh_interval_us);
  if (err) return err;

  err = driver_ops->config_channel(pd, &config);
  if (err) return err;

  err = driver_ops->start_DMA(pd, points, len, DEVICE_DAC_ALIGN_12B_R);
  if (err) return err;

  err = pd->timer->ops->start(pd->timer, DEVICE_TIMER_START_MODE_NO_IT);
  if (err) return err;

  pd->run_status = DEVICE_DAC_RUN_STATUS_WAVE;

  return ESUCCESS;
}

