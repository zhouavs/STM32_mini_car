#include "dac.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "common/errno/errno.h"
#include "device_config/gpio/gpio.h"
#include "device_config/usart/usart.h"
#include "device_config/spi/spi.h"
#include "device_config/st7789v2/st7789v2.h"
#include "device_config/timer/timer.h"
#include "device_config/dac/dac.h"
#include "common/delay/delay.h"
#include "stm32f4xx_hal.h"
#include "Core/Inc/dac.h"

static errno_t init(void);
static void generate_sine_wave(uint16_t *points, uint16_t point_num, uint16_t amplitude, uint16_t offset);

void dac_test() {
  #define WIDTH 200
  #define HEIGHT 50
  #define ONE_PIXEL_BYTE_NUM 2

  const uint32_t display_memory_size = WIDTH * HEIGHT * ONE_PIXEL_BYTE_NUM;
  uint8_t display_memory[WIDTH * HEIGHT * ONE_PIXEL_BYTE_NUM] = {0};

  #define POINT_COUNT 100
  // DMA 循环模式需要持久内存，不能用栈数组
  static uint16_t points[POINT_COUNT] = {0};
  generate_sine_wave(points, POINT_COUNT, 0xFFF - 0xB00, 0xB00);

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

  Device_DAC *pdd = NULL;
  err = Device_DAC_find(&pdd, DEVICE_DAC_LIGHT);
  if (err) goto print_err_tag;
  err = pdd->ops->init(pdd);
  if (err) goto print_err_tag;
  err = pdd->ops->set_wave(pdd, points, POINT_COUNT, 20000);
  if (err) goto print_err_tag;

  uint8_t str[50] = {0};

  while (1) {
    err = pds->ops->fill_window(pds, 0xfff0);
    if (err) goto print_err_tag;
    uint16_t v = (uint16_t)HAL_DAC_GetValue(&hdac, DAC_CHANNEL_1);
    snprintf((char *)str, 50, "DAC value: %d", v);
    err = pds->ops->set_ascii_str(pds, str, strlen((char *)str), 0, 0, 0);
    if (err) goto print_err_tag;
    err = pds->ops->refresh_window(pds);
    if (err) goto print_err_tag;
    delay_ms(100);
  }

  print_err_tag:
  printf("dac_test_err\r\nerr: %d\r\n", err);
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
  
  err = Device_config_DAC_register_all_device();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("st7789v2_test_init_err\r\nerr: %d\r\n", err);
  return err;
}

static void generate_sine_wave(uint16_t *points, uint16_t point_num, uint16_t amplitude, uint16_t offset) {
  #define PI 3.14159265358979f

  for (uint16_t i = 0; i < point_num; ++i) {
    float angle = 2.0f * PI * i / point_num;
    points[i] = (uint16_t)(amplitude * (sinf(angle) + 1.0f) / 2.0f + offset);
  }
}
