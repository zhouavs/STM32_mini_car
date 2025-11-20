#include "at24c02.h"
#include "device/at24c02/at24c02.h"
#include "device/gpio/gpio.h"
#include "device/i2c/i2c.h"
#include <stdlib.h>

static Device_AT24C02 devices[DEVICE_AT24C02_COUNT] = {
  [DEVICE_AT24C02_1] = {
    .name = DEVICE_AT24C02_1,
  },
};
// 关联的 I2c
static const Device_I2C_name relate_i2c[DEVICE_AT24C02_COUNT] = {
  [DEVICE_AT24C02_1] = DEVICE_I2C_1,
};

errno_t Device_config_AT24C02_register_all_device(void) {
  for (Device_AT24C02_name name = 0; name < DEVICE_AT24C02_COUNT; ++name) {
    errno_t err = ESUCCESS;

    err = Device_I2C_find(&devices[name].i2c, relate_i2c[name]);
    if (err) return err;

    err = Device_AT24C02_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
