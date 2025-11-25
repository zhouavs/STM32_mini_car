#include "at24c02.h"
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
#include "device/i2c/i2c.h"
#include "device_config/i2c/i2c.h"
#include "device/at24c02/at24c02.h"
#include "device_config/at24c02/at24c02.h"

static errno_t init(void);

void at24c02_test() {
  errno_t err = init();
  if (err) goto print_err_tag;

  Device_ST7789V2 *pds = NULL;
  err = Device_ST7789V2_find(&pds, DEVICE_ST7789V2_1);
  if (err) goto print_err_tag;
  
  const Device_AT24C02 *pda = NULL;
  err = Device_AT24C02_find(&pda, DEVICE_AT24C02_1);
  if (err) goto print_err_tag;

  err = pds->ops->init(pds);
  if (err) goto print_err_tag;

  err = pda->ops->init(pda);
  if (err) goto print_err_tag;

  err = pds->ops->clear_screen(pds, 0xf7b6);
  if (err) goto print_err_tag;

  const uint32_t display_height = 200, display_width = 200;
  const uint32_t display_memory_size = display_height * display_width * pds->one_pixel_byte_num;
  uint8_t *display_memory = (uint8_t *)calloc(display_memory_size, sizeof(uint8_t));
  if (display_memory == NULL) {
    err = ENOMEM;
    goto print_err_tag;
  }

  err = pds->ops->set_display_memory(pds, display_memory, display_memory_size);
  if (err) goto print_err_tag;

  err = pds->ops->set_window(pds, 10, 10, 10 + display_height - 1, 10 + display_width - 1);
  if (err) goto print_err_tag;

  err = pds->ops->fill_window(pds, 0xf628);
  if (err) goto print_err_tag;

  err = pds->ops->refresh_window(pds);
  if (err) goto print_err_tag;

  const uint16_t w_data_len = 27;
  uint8_t *w_data = (uint8_t *)calloc(w_data_len, sizeof(uint8_t));
  if (w_data == NULL) {
    err = ENOMEM;
    goto print_err_tag;
  }
  for (uint16_t i = 0; i < w_data_len - 1; ++i) {
    w_data[i] = 'a' + i;
  }
  err = pda->ops->write(pda, 0x00, w_data, w_data_len - 1);
  if (err) goto print_err_tag;

  const uint16_t r_data_len = 27;
  uint8_t *r_data = (uint8_t *)calloc(r_data_len, sizeof(uint8_t));
  if (r_data == NULL) {
    err = ENOMEM;
    goto print_err_tag;
  }
  err = pda->ops->read(pda, 0x00, r_data, r_data_len);
  if (err) goto print_err_tag;

  err = pds->ops->set_ascii_str(pds, r_data, r_data_len - 1, 0, 0, 0x00);
  if (err) goto print_err_tag;

  err = pds->ops->refresh_window(pds);
  if (err) goto print_err_tag;

  while (1);

  print_err_tag:
  printf("at24c02_test_err\r\nerr: %d\r\n", err);
  while (1);

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

  err = Device_I2C_module_init();
  if (err) goto print_err_tag;

  err = Device_config_I2C_register_all_device();
  if (err) goto print_err_tag;

  err = Device_AT24C02_module_init();
  if (err) goto print_err_tag;

  err = Device_config_AT24C02_register_all_device();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("st7789v2_test_init_err\r\nerr: %d\r\n", err);
  return err;
}
