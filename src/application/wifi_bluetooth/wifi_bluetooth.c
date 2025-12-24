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
#include "device_config/keyboard/keyboard.h"
#include "common/delay/delay.h"

static errno_t init(void);

static const uint8_t line_height = 16;

void wifi_bluetooth_test() {
  #define WIDTH 150
  #define HEIGHT 200
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

  const Device_keyboard *pdk = NULL;
  err = Device_keyboard_find(&pdk, DEVICE_KEYBOARD_1);
  if (err) goto print_err_tag;

  uint8_t step = 1;
  for (;;) {
    switch (step) {
      case 1: {
        Device_key_name key;
        err = pdk->ops->read(&key);
        if (err == ENODATA) continue;
        if (err) goto print_err_tag;
        if (key != DEVICE_KEY_1) continue;

        // 连接 wifi
        err = pdw->ops->join_wifi_ap(pdw, (uint8_t *)"Law_of_Cycles", (uint8_t *)"Homura_9630");
        if (err)
          goto print_err_tag;

        // 新建 socket 链接
        err = pdw->ops->create_socket_connection(pdw, TCP_CLIENT, (uint8_t *)"124.156.213.226", 9000);
        if (err)
          goto print_err_tag;

        err = delay_s(1);
        if (err) goto print_err_tag;

        // 接收消息
        uint8_t read_buf[100] = {0};
        uint32_t read_len = 0;
        err = pdw->ops->socket_read(pdw, 9000, read_buf, &read_len, 100);
        if (err) goto print_err_tag;
        err = pds->ops->set_ascii_str(pds, read_buf, read_len, line_height * 0, 0, 0x0000);
        if (err) goto print_err_tag;
        err = pds->ops->refresh_window(pds);
        if (err) goto print_err_tag;

        step = 2;

        break;
      }
      case 2: {
        Device_key_name key;
        err = pdk->ops->read(&key);
        if (err == ENODATA) continue;
        if (err) goto print_err_tag;
        if (key != DEVICE_KEY_2) continue;

        // 清除显示区
        err = pds->ops->fill_window(pds, 0xfff0);
        if (err) goto print_err_tag;
        err = pds->ops->refresh_window(pds);
        if (err) goto print_err_tag;

        uint8_t read_buf[100] = {0};
        uint32_t read_len = 0;

        // 发送消息
        const char *send_msg_1 = "ping\r\n";
        err = pdw->ops->socket_send(pdw, 9000, (uint8_t *)send_msg_1, strlen((char *)send_msg_1));
        if (err) goto print_err_tag;
        err = delay_s(1);
        if (err) goto print_err_tag;

        // 接收消息
        err = pdw->ops->socket_read(pdw, 9000, read_buf, &read_len, 100);
        if (err) goto print_err_tag;
        err = pds->ops->set_ascii_str(pds, read_buf, read_len, line_height * 1, 0, 0x0000);
        if (err) goto print_err_tag;
        err = pds->ops->refresh_window(pds);
        if (err) goto print_err_tag;

        // 发送消息
        const char *send_msg_2 = "madoka\r\nhomura\r\n";
        err = pdw->ops->socket_send(pdw, 9000, (uint8_t *)send_msg_2, strlen((char *)send_msg_2));
        if (err) goto print_err_tag;
        err = delay_s(1);
        if (err) goto print_err_tag;

        // 接收消息
        err = pdw->ops->socket_read(pdw, 9000, read_buf, &read_len, 100);
        if (err) goto print_err_tag;
        err = pds->ops->set_ascii_str(pds, read_buf, read_len, line_height * 2, 0, 0x0000);
        if (err) goto print_err_tag;
        err = pds->ops->refresh_window(pds);
        if (err) goto print_err_tag;

        // 发送消息
        const char *send_msg_3 = "\r\n";
        err = pdw->ops->socket_send(pdw, 9000, (uint8_t *)send_msg_3, strlen((char *)send_msg_3));
        if (err) goto print_err_tag;
        err = delay_s(1);
        if (err) goto print_err_tag;

        // 接收消息
        err = pdw->ops->socket_read(pdw, 9000, read_buf, &read_len, 100);
        if (err) goto print_err_tag;
        err = pds->ops->set_ascii_str(pds, read_buf, read_len, line_height * 4, 0, 0x0000);
        if (err) goto print_err_tag;
        err = pds->ops->refresh_window(pds);
        if (err) goto print_err_tag;

        break;
      }
    }
  }

  print_err_tag:
  printf("wifi_bluetooth_test_err\r\nerr: %d\r\n", err);
  for (;;);

  return;
}

static errno_t init(void) {
  errno_t err = ESUCCESS;

  err = Device_config_GPIO_register();
  if (err) goto print_err_tag;

  err = Device_config_USART_register();
  if (err) return err;

  err = Device_config_timer_register();
  if (err) goto print_err_tag;

  err = Device_config_SPI_register();
  if (err) goto print_err_tag;

  err = Device_config_ST7789V2_register();
  if (err) goto print_err_tag;
  
  err = Device_config_wifi_bluetooth_register();
  if (err) goto print_err_tag;

  err = Device_config_keyboard_register();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("wifi_bluetooth_test_init_err\r\nerr: %d\r\n", err);
  return err;
}
