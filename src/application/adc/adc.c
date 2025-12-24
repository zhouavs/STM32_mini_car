#include "adc.h"
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
#include "device_config/adc/adc.h"
#include "common/delay/delay.h"

static errno_t init(void);

void adc_test() {
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

  Device_ADC *pdp = NULL, *pdl = NULL;
  err = Device_ADC_find(&pdp, DEVICE_ADC_POWER);
  if (err) goto print_err_tag;
  err = pdp->ops->init(pdp);
  if (err) goto print_err_tag;
  err = Device_ADC_find(&pdl, DEVICE_ADC_LIGHT);
  if (err) goto print_err_tag;
  err = pdl->ops->init(pdl);
  if (err) goto print_err_tag;

  uint16_t power_value = 0, light_value = 0;
  uint8_t str[50] = {0};

  while (1) {
    err = pds->ops->fill_window(pds, 0xfff0);
    if (err) goto print_err_tag;

    err = pdp->ops->read(pdp, &power_value, 1);
    if (err) goto print_err_tag;
    snprintf((char *)str, 50, "power value: %d", power_value);
    err = pds->ops->set_ascii_str(pds, str, strlen((char *)str), 0, 0, 0x0000);
    if (err) goto print_err_tag;

    err = pdl->ops->read(pdl, &light_value, 1);
    if (err) goto print_err_tag;
    snprintf((char *)str, 50, "light value: %d", light_value);
    err = pds->ops->set_ascii_str(pds, str, strlen((char *)str), 16, 0, 0x0000);
    if (err) goto print_err_tag;

    err = pds->ops->refresh_window(pds);
    if (err) goto print_err_tag;

    delay_s(1);
  }

  print_err_tag:
  printf("adc_test_err\r\nerr: %d\r\n", err);
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
  
  err = Device_config_ADC_register();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("st7789v2_test_init_err\r\nerr: %d\r\n", err);
  return err;
}
