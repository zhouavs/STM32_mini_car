#include "rtc.h"
#include "stm32f4xx_hal.h"
#include "driver/rtc/rtc.h"
#include "Core/Inc/rtc.h"

static Device_RTC devices[DEVICE_RTC_COUNT] = {
  [DEVICE_RTC_1] = {
    .name = DEVICE_RTC_1,
    .instance = &hrtc,
  },
};

errno_t Device_config_RTC_register_all_device(void) {
  errno_t err = Device_RTC_module_init();
  if (err) return err;
  
  for (Device_RTC_name name = 0; name < DEVICE_RTC_COUNT; ++name) {
    err = Device_RTC_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
