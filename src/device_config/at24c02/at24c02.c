#include "at24c02.h"
#include "device/gpio/gpio.h"
#include "device/i2c/i2c.h"
#include <stdlib.h>

static Device_AT24C02 devices[DEVICE_AT24C02_COUNT] = {
  [DEVICE_AT24C02_1] = {
    .name = DEVICE_AT24C02_1,
    .addr = 0x50,
    .size = 0x100,
    .page_size = 0x08,
    .page_count = 0x20,
  },
};
// 关联的 I2c
static const Device_I2C_name relate_i2c[DEVICE_AT24C02_COUNT] = {
  [DEVICE_AT24C02_1] = DEVICE_I2C_1,
};

errno_t Device_config_AT24C02_register_all_device(void) {
  errno_t err = Device_AT24C02_module_init();
  if (err) return err;
  
  for (Device_AT24C02_name name = 0; name < DEVICE_AT24C02_COUNT; ++name) {

    err = Device_I2C_find(&devices[name].i2c, relate_i2c[name]);
    if (err) return err;

    err = Device_AT24C02_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
