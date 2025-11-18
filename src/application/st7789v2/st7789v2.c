#include "st7789v2.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device_config/gpio/gpio.h"
#include "device/usart/usart.h"
#include "device_config/usart/usart.h"
#include "device/spi/spi.h"
#include "device_config/spi/spi.h"
#include "device/w25qx/w25qx.h"
#include "device_config/w25qx/w25qx.h"
#include "device/st7789v2/st7789v2.h"
#include "device_config/st7789v2/st7789v2.h"

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define ONE_PIXEL_BYTE_NUM 2

static errno_t init(void);
static errno_t clear_screen(uint16_t color);

void st7789v2_test() {
  errno_t err = init();
  if (err) return;

  Device_ST7789V2 *pds = NULL;
  err = Device_ST7789V2_find(&pds, DEVICE_ST7789V2_1);
  if (err) goto print_err_tag;

  err = pds->ops->init(pds);
  if (err) goto print_err_tag;

  err = clear_screen(0xffff);
  if (err) goto print_err_tag;

  uint8_t pixel_bytes[40 * 40 * 2] = {0};
  pds->pixel_bytes = pixel_bytes;
  pds->pixel_bytes_size = 40 * 40 * 2;

  err = pds->ops->set_window(pds, 10, 20, 49, 59);
  if (err) goto print_err_tag;

  for (uint16_t y = 0; y < 40; ++y) {
    for (uint16_t x = 0; x < 40; ++x) {
      err = pds->ops->set_pixel(pds, y, x, 0x8f4a);
      if (err) {
        goto print_err_tag;
      }
    }
  }

  err = pds->ops->refresh_window(pds);
  if (err) goto print_err_tag;

  while (1) {}

  print_err_tag:
  printf("st7789v2_test_err\r\nerr: %d\r\n", err);
  return;
}

static errno_t init(void) {
  errno_t err = ESUCCESS;

  err = Device_USART_module_init();
  if (err) return err;

  err = Device_config_USART_register_all_device();
  if (err) return err;

  err = Device_GPIO_module_init();
  if (err) goto print_err_tag;

  err = Device_config_GPIO_register_all_device();
  if (err) goto print_err_tag;

  err = Device_SPI_module_init();
  if (err) goto print_err_tag;

  err = Device_config_SPI_register_all_device();
  if (err) goto print_err_tag;

  err = Device_W25QX_module_init();
  if (err) goto print_err_tag;

  err = Device_config_W25QX_register_all_device();
  if (err) goto print_err_tag;

  err = Device_ST7789V2_module_init();
  if (err) goto print_err_tag;

  err = Device_config_ST7789V2_register_all_device();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("st7789v2_test_init_err\r\nerr: %d\r\n", err);
  return err;
}

static errno_t clear_screen(uint16_t color) {
  Device_ST7789V2 *pds = NULL;
  errno_t err = Device_ST7789V2_find(&pds, DEVICE_ST7789V2_1);
  if (err) return err;

  uint8_t pixel_bytes[SCREEN_WIDTH * 1 * ONE_PIXEL_BYTE_NUM] = {0};
  pds->pixel_bytes = pixel_bytes;
  pds->pixel_bytes_size = SCREEN_WIDTH * 1 * ONE_PIXEL_BYTE_NUM;

  err = pds->ops->set_window(pds, 0, 0, 0, SCREEN_WIDTH - 1);
  if (err) return err;

  for (uint16_t x = 0; x < SCREEN_WIDTH; ++x) {
    err = pds->ops->set_pixel(pds, 0, x, color);
    if (err) return err;
  }
  
  err = pds->ops->refresh_window(pds);
  if (err) return err;

  for (uint16_t y = 0; y < SCREEN_HEIGHT; ++y) {
    err = pds->ops->set_window(pds, y, 0, y, SCREEN_WIDTH - 1);
    if (err) {
      return err;
    }

    err = pds->ops->refresh_window(pds);
    if (err) {
      return err;
    }
  }

  return ESUCCESS;
}
