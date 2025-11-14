#include "w25qx.h"
#include "device/w25qx/w25qx.h"
#include "device/gpio/gpio.h"
#include "device/spi/spi.h"
#include <stdlib.h>

static Device_W25QX devices[DEVICE_W25QX_COUNT] = {
  [DEVICE_W25Q64] = {0},
};
// 关联的 GPIO
const Device_GPIO_name relate_GPIO[DEVICE_W25QX_COUNT] = {
  [DEVICE_W25Q64] = DEVICE_W25Q64_CS,
};
// 关联的 SPI
const Device_SPI_name relate_SPI[DEVICE_W25QX_COUNT] = {
  [DEVICE_W25Q64] = DEVICE_SPI_1,
};

errno_t Device_config_W25QX_register_all_device(void) {
  for (Device_W25QX_name name = 0; name < DEVICE_W25QX_COUNT; ++name) {
    devices[name].name = name;

    errno_t err = ESUCCESS;

    err = Device_GPIO_find(&devices[name].cs, relate_GPIO[name]);
    if (err) return err;
    err = Device_SPI_find(&devices[name].spi, relate_SPI[name]);
    if (err) return err;

    err = Device_W25QX_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
