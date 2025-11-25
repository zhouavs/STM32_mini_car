#include "tracker.h"
#include "device/gpio/gpio.h"
#include <stdlib.h>

static Device_tracker devices[DEVICE_TRACKER_COUNT] = {
  [DEVICE_TRACKER_1] = {
    .name = DEVICE_TRACKER_1,
  },
};
// 关联 ins 设备
static const Device_GPIO_name relate_ins[DEVICE_TRACKER_COUNT][DEVICE_TRACKER_IN_COUNT] = {
  [DEVICE_TRACKER_1] = {DEVICE_TRACKER_IN_1, DEVICE_TRACKER_IN_2, DEVICE_TRACKER_IN_3, DEVICE_TRACKER_IN_4, DEVICE_TRACKER_IN_5, DEVICE_TRACKER_IN_6, DEVICE_TRACKER_IN_7},
};

errno_t Device_config_tracker_register_all_device(void) {
  errno_t err = Device_tracker_module_init();
  if (err) return err;
  
  for (Device_tracker_name name = 0; name < DEVICE_TRACKER_COUNT; ++name) {
    for (uint8_t i = 0; i < DEVICE_TRACKER_IN_COUNT; ++i) {
      err = Device_GPIO_find(&devices[name].ins[i], relate_ins[name][i]);
    }
    
    err = Device_tracker_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
