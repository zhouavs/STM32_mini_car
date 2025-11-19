#include "font.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

static errno_t init(void);

void font_test(void) {
  errno_t err = init();
  if (err) return;

  Device_ST7789V2 *pds = NULL;
  err = Device_ST7789V2_find(&pds, DEVICE_ST7789V2_1);
  if (err) goto print_err_tag;

  err = pds->ops->init(pds);
  if (err) goto print_err_tag;

  err = pds->ops->clear_screen(pds, 0xf628);
  if (err) goto print_err_tag;
  
  const uint32_t display_memory_size = 100 * 100 * 2;
  uint8_t *const display_memory = (uint8_t *)calloc(display_memory_size, sizeof(uint8_t));

  err = pds->ops->set_display_memory(pds, display_memory, display_memory_size);
  if (err) goto print_err_tag;
  
  err = pds->ops->set_window(pds, 20, 10, 120 - 1, 110 - 1);
  if (err) goto print_err_tag;
  
  err = pds->ops->fill_window(pds, 0xf628);
  if (err) goto print_err_tag;

  const char *const str = "abcABCZYXzyx\r\nHIG\r\nOPQ\r\n1234\r5\n6";
  err = pds->ops->set_ascii_str(pds, (uint8_t *)str, strlen(str), 0, 0, 0x0000, 0xf628);
  if (err) goto print_err_tag;

  err = pds->ops->refresh_window(pds);
  if (err) goto print_err_tag;

  while (1);

  print_err_tag:
  printf("st7789v2_test_err\r\nerr: %d\r\n", err);
  while (1);
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

