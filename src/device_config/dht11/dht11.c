#include "dht11.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdlib.h>

static Device_DHT11 devices[DEVICE_DHT11_COUNT] = {
  [DEVICE_DHT11_1] = {
    .name = DEVICE_DHT11_1,
  },
};
// 关联 gpio 设备
static const Device_GPIO_name relate_in[DEVICE_DHT11_COUNT] = {
  [DEVICE_DHT11_1] = DEVICE_DHT11_IN,
};
// 关联计时器
static const Device_timer_name relate_timer[DEVICE_DHT11_COUNT] = {
  [DEVICE_DHT11_1] = DEVICE_TIMER_TIM10,
};

errno_t Device_config_DHT11_register_all_device(void) {
  errno_t err = Device_DHT11_module_init();
  if (err) return err;
  
  for (Device_DHT11_name name = 0; name < DEVICE_DHT11_COUNT; ++name) {
    err = Device_GPIO_find(&devices[name].in, relate_in[name]);
    if (err) return err;

    err = Device_timer_find(&devices[name].timer, relate_timer[name]);
    if (err) return err;

    err = Device_DHT11_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
