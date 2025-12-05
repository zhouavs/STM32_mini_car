#include "wifi_bluetooth.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common/errno/errno.h"
#include "device_config/gpio/gpio.h"
#include "device_config/usart/usart.h"
#include "device_config/spi/spi.h"
#include "device_config/st7789v2/st7789v2.h"
#include "device_config/timer/timer.h"
#include "device_config/wifi_bluetooth/wifi_bluetooth.h"
#include "common/delay/delay.h"

static errno_t init(void);

void wifi_bluetooth_test() {
  #define WIDTH 100
  #define HEIGHT 100
  #define ONE_PIXEL_BYTE_NUM 2

  const uint32_t display_memory_size = WIDTH * HEIGHT * ONE_PIXEL_BYTE_NUM;
  uint8_t display_memory[WIDTH * HEIGHT * ONE_PIXEL_BYTE_NUM] = {0};

  errno_t err = init();
  if (err) goto print_err_tag;

  Device_ST7789V2 *pds = NULL;
  err = Device_ST7789V2_find(&pds, DEVICE_ST7789V2_1);
  if (err) goto print_err_tag;
  err = pds->ops->init(pds);
  if (err) goto print_err_tag;
  err = pds->ops->clear_screen(pds, 0xFFFF);
  if (err) goto print_err_tag;
  err = pds->ops->set_display_memory(pds, display_memory, display_memory_size);
  if (err) goto print_err_tag;
  err = pds->ops->set_window(pds, 10, 10, 10 + HEIGHT - 1, 10 + WIDTH - 1);
  if (err) goto print_err_tag;
  err = pds->ops->fill_window(pds, 0xfff0);
  if (err) goto print_err_tag;
  err = pds->ops->refresh_window(pds);
  if (err) goto print_err_tag;

  Device_wifi_bluetooth *pdw = NULL;
  err = Device_wifi_bluetooth_find(&pdw, DEVICE_WIFI_BLUETOOTH_1);
  if (err) goto print_err_tag;
  err = pdw->ops->init(pdw);
  if (err) goto print_err_tag;

  // 链接 wifi
  err = pdw->ops->join_wifi_ap(pdw, (uint8_t *)"Law_of_Cycles", (uint8_t *)"Homura_9630");
  if (err) goto print_err_tag;

  // 新建 socket 链接
  err = pdw->ops->create_socket_connection(pdw, TCP_CLIENT, (uint8_t *)"192.168.31.109", 9000);
  if (err) goto print_err_tag;

  // 发送消息
  const char *send_msg = "ping";
  err = pdw->ops->socket_send(pdw, 9000, (uint8_t *)send_msg, strlen((char *)send_msg));
  if (err) goto print_err_tag;

  // 接收消息
  uint8_t read_buf[100] = {0};
  uint32_t read_len = 0;
  err = pdw->ops->socket_read(pdw, 9000, read_buf, &read_len, 100);
  if (err) goto print_err_tag;

  while (1) {

  }

  print_err_tag:
  printf("wifi_bluetooth_test_err\r\nerr: %d\r\n", err);
  while (1);

  return;
}

static errno_t init(void) {
  errno_t err = ESUCCESS;

  err = Device_config_GPIO_register_all_device();
  if (err) goto print_err_tag;

  err = Device_config_USART_register_all_device();
  if (err) return err;

  err = Device_config_timer_register_all_device();
  if (err) goto print_err_tag;

  err = Device_config_SPI_register_all_device();
  if (err) goto print_err_tag;

  err = Device_config_ST7789V2_register_all_device();
  if (err) goto print_err_tag;
  
  err = Device_config_wifi_bluetooth_register_all_device();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("st7789v2_test_init_err\r\nerr: %d\r\n", err);
  return err;
}
