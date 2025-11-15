#include "w25qx.h"
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

static errno_t init(void);

void w25q64_test() {
  errno_t err = init();
  if (err) return;

  const Device_W25QX *pw = NULL;
  err = Device_W25QX_find(&pw, DEVICE_W25Q64);
  if (err) return;

  #define G_W_DATA_LEN 0x2001
  uint8_t g_w_data[G_W_DATA_LEN] = {
    [0] = 'k', [1] = 'y', [2] = 'o', [3] = 't', [4] = 'o',
    [0xFF] = '_', [0x100] = 'a', [0x101] = 'n', [0xFFF] = 'i', [0x1000] = 'm', [0x2000] = 'e',
  };
  #define G_R_DATA_LEN 0x2010
  uint8_t g_r_data[G_R_DATA_LEN + 1] = {0};

  err = pw->ops->write(pw, 3, g_w_data, G_W_DATA_LEN);
  if (err) {
    printf("write fail: %d\r\n", err);
    return;
  }
  printf("write success\r\n");
  
  err = pw->ops->read(pw, 0, g_r_data, G_R_DATA_LEN);
  if (err) {
    printf("read fail: %d\r\n", err);
    return;
  }
  printf("read success\r\n");
  printf("read content: %s\r\n", g_r_data);

  while (1) {
  
  }

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

  return ESUCCESS;

  print_err_tag:
  printf("w25q64 test err\r\nerr: %d", err);
  return err;
}
