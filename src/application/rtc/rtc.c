#include "rtc.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common/errno/errno.h"
#include "device_config/gpio/gpio.h"
#include "device_config/usart/usart.h"
#include "device_config/spi/spi.h"
#include "device_config/st7789v2/st7789v2.h"
#include "device_config/timer/timer.h"
#include "device_config/rtc/rtc.h"
#include "common/delay/delay.h"

static errno_t init(void);

void rtc_test() {
  #define WIDTH 200
  #define HEIGHT 50
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

  Device_RTC *pdu = NULL;
  err = Device_RTC_find(&pdu, DEVICE_RTC_1);
  if (err) goto print_err_tag;
  err = pdu->ops->init(pdu);
  if (err) goto print_err_tag;
  Device_RTC_date_time dt = {0};

  while (1) {
    err = pdu->ops->get_date_time(pdu, &dt);
    if (err) goto print_err_tag;
    uint8_t str[50] = {0};
    snprintf((char *)str, 50, "20%02d-%02d-%02d %02d:%02d:%02d", dt.date.year, dt.date.month, dt.date.day, dt.time.hour, dt.time.minute, dt.time.second);
    err = pds->ops->fill_window(pds, 0xfff0);
    if (err) goto print_err_tag;
    err = pds->ops->set_ascii_str(pds, str, strlen((char *)str), 0, 0, 0x0000);
    if (err) goto print_err_tag;
    err = pds->ops->refresh_window(pds);
    if (err) goto print_err_tag;
    err = delay_s(1);
    if (err) goto print_err_tag;
  }

  print_err_tag:
  printf("irda_test_err\r\nerr: %d\r\n", err);
  while (1);

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
  
  err = Device_config_RTC_register();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("st7789v2_test_init_err\r\nerr: %d\r\n", err);
  return err;
}
