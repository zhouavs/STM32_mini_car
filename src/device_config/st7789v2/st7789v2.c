#include "st7789v2.h"
#include "device/st7789v2/st7789v2.h"
#include "device/gpio/gpio.h"
#include "device/spi/spi.h"
#include <stdlib.h>

static Device_ST7789V2 devices[DEVICE_ST7789V2_COUNT] = {
  [DEVICE_ST7789V2_1] = {
    .one_pixel_byte_num = 2, // 目前只支持 16 位 RGB 为 565 的像素格式
  },
};
// 关联的 CS 片选线
static const Device_GPIO_name relate_cs[DEVICE_ST7789V2_COUNT] = {
  [DEVICE_ST7789V2_1] = DEVICE_ST7789V2_1_CS,
};
// 关联的 RST 硬重置线
static const Device_GPIO_name relate_rst[DEVICE_ST7789V2_COUNT] = {
  [DEVICE_ST7789V2_1] = DEVICE_ST7789V2_1_RST,
};
// 关联的 DC 数据/指令线
static const Device_GPIO_name relate_dc[DEVICE_ST7789V2_COUNT] = {
  [DEVICE_ST7789V2_1] = DEVICE_ST7789V2_1_DC,
};
// 关联的 SPI
static const Device_SPI_name relate_spi[DEVICE_ST7789V2_COUNT] = {
  [DEVICE_ST7789V2_1] = DEVICE_SPI_1,
};

errno_t Device_config_ST7789V2_register_all_device(void) {
  for (Device_ST7789V2_name name = 0; name < DEVICE_ST7789V2_COUNT; ++name) {
    devices[name].name = name;

    errno_t err = ESUCCESS;

    err = Device_GPIO_find(&devices[name].cs, relate_cs[name]);
    if (err) return err;
    err = Device_GPIO_find(&devices[name].dc, relate_dc[name]);
    if (err) return err;
    err = Device_GPIO_find(&devices[name].rst, relate_rst[name]);
    if (err) return err;
    err = Device_SPI_find(&devices[name].spi, relate_spi[name]);
    if (err) return err;

    err = Device_ST7789V2_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
