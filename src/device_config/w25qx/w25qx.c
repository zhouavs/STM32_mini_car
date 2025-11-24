#include "w25qx.h"
#include "device/gpio/gpio.h"
#include "device/spi/spi.h"
#include <stdlib.h>

static Device_W25QX devices[DEVICE_W25QX_COUNT] = {
  [DEVICE_W25Q64] = {
    .name = DEVICE_W25Q64,
  },
};
// 关联的 GPIO
static const Device_GPIO_name relate_cs[DEVICE_W25QX_COUNT] = {
  [DEVICE_W25Q64] = DEVICE_W25Q64_CS,
};
// 关联的 SPI
static const Device_SPI_name relate_spi[DEVICE_W25QX_COUNT] = {
  [DEVICE_W25Q64] = DEVICE_SPI_1,
};

errno_t Device_config_W25QX_register_all_device(void) {
  errno_t err = Device_W25QX_module_init();
  if (err) return err;
  
  for (Device_W25QX_name name = 0; name < DEVICE_W25QX_COUNT; ++name) {
    err = Device_GPIO_find(&devices[name].cs, relate_cs[name]);
    if (err) return err;
    err = Device_SPI_find(&devices[name].spi, relate_spi[name]);
    if (err) return err;

    err = Device_W25QX_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
