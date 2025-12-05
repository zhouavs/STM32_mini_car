#include "wifi_bluetooth.h"
#include "device/timer/timer.h"
#include "device/usart/usart.h"
#include <stdlib.h>

static Device_wifi_bluetooth devices[DEVICE_WIFI_BLUETOOTH_COUNT] = {
  [DEVICE_WIFI_BLUETOOTH_1] = {
    .name = DEVICE_WIFI_BLUETOOTH_1,
  },
};
// 关联串口设备
static const Device_USART_name relate_usart[DEVICE_WIFI_BLUETOOTH_COUNT] = {
  [DEVICE_WIFI_BLUETOOTH_1] = DEVICE_USART_WIFI_BLUETOOTH,
};
// 关联定时器设备
static const Device_timer_name relate_timer[DEVICE_WIFI_BLUETOOTH_COUNT] = {
  [DEVICE_WIFI_BLUETOOTH_1] = DEVICE_TIMER_TIM13,
};


errno_t Device_config_wifi_bluetooth_register_all_device(void) {
  errno_t err = Device_wifi_bluetooth_module_init();
  if (err) return err;

  for (Device_wifi_bluetooth_name name = 0; name < DEVICE_WIFI_BLUETOOTH_COUNT; ++name) {
    err = Device_USART_find(&devices[name].usart, relate_usart[name]);
    if (err) return err;
    err = Device_timer_find(&devices[name].timer, relate_timer[name]);
    if (err) return err;
    err = Device_wifi_bluetooth_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
