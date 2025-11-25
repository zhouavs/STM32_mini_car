#include "speed_test.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "common/delay/delay.h"
#include "device_config/gpio/gpio.h"
#include "device_config/timer/timer.h"
#include "device_config/pwm/pwm.h"
#include "device_config/spi/spi.h"
#include "device_config/st7789v2/st7789v2.h"
#include "device_config/motor/motor.h"
#include "device_config/speed_test/speed_test.h"

static errno_t init(void);

void speed_test_test(void) {
  errno_t err = ESUCCESS;

  err = init();
  if (err) goto err_tag;

  Device_ST7789V2 *pds = NULL;
  err = Device_ST7789V2_find(&pds, DEVICE_ST7789V2_1);
  if (err) goto err_tag;

  Device_motor *pdm_hl = NULL, *pdm_hr = NULL, *pdm_tl = NULL, *pdm_tr = NULL;

  err = Device_motor_find(&pdm_hl, DEVICE_MOTOR_HEAD_LEFT);
  if (err) goto err_tag;

  err = Device_motor_find(&pdm_hr, DEVICE_MOTOR_HEAD_RIGHT);
  if (err) goto err_tag;

  err = Device_motor_find(&pdm_tl, DEVICE_MOTOR_TAIL_LEFT);
  if (err) goto err_tag;

  err = Device_motor_find(&pdm_tr, DEVICE_MOTOR_TAIL_RIGHT);
  if (err) goto err_tag;

  Device_speed_test *pdst_hl = NULL, *pdst_hr = NULL, *pdst_tl = NULL, *pdst_tr = NULL;

  err = Device_speed_test_find(&pdst_hl, DEVICE_SPEED_TEST_HEAD_LEFT);
  if (err) goto err_tag;

  err = Device_speed_test_find(&pdst_hr, DEVICE_SPEED_TEST_HEAD_RIGHT);
  if (err) goto err_tag;

  err = Device_speed_test_find(&pdst_tl, DEVICE_SPEED_TEST_TAIL_LEFT);
  if (err) goto err_tag;

  err = Device_speed_test_find(&pdst_tr, DEVICE_SPEED_TEST_TAIL_RIGHT);
  if (err) goto err_tag;

  err = pds->ops->init(pds);
  if (err) goto err_tag;

  err = pds->ops->clear_screen(pds, 0xFFFF);
  if (err) goto err_tag;

  #define WIDTH 100
  #define HEIGHT 100
  const uint32_t display_memory_size = WIDTH * HEIGHT * 2;
  uint8_t display_memory[WIDTH * HEIGHT * 2] = {0};

  err = pds->ops->set_display_memory(pds, display_memory, display_memory_size);
  if (err) goto err_tag;

  err = pds->ops->set_window(pds, 10, 10, 10 + WIDTH - 1, 10 + HEIGHT -1);
  if (err) goto err_tag;

  const uint16_t window_background_color = 0xfff4;

  err = pds->ops->fill_window(pds, window_background_color);
  if (err) goto err_tag;

  err = pds->ops->refresh_window(pds);
  if (err) goto err_tag;

  err = pdm_hl->ops->init(pdm_hl);
  if (err) goto err_tag;

  err = pdm_hl->ops->forward(pdm_hl, 0x30);
  if (err) goto err_tag;

  err = pdst_hl->ops->init(pdst_hl);
  if (err) goto err_tag;

  float speed = 0;
  uint8_t speed_str[20] = {0};

  while (1) {
    err = pdst_hl->ops->get_speed(pdst_hl, &speed);
    if (err) goto err_tag;

    snprintf((char *)speed_str, 20, "speed: %0.3f", speed);
    err = pds->ops->set_ascii_str(pds, speed_str, strlen((char *)speed_str), 0, 0, 0x0000);
    if (err) goto err_tag;

    err = pds->ops->refresh_window(pds);
    if (err) goto err_tag;

    delay_s(1);
  }

  err_tag:
  while (1);
}

static errno_t init(void) {
  errno_t err = ESUCCESS;

  err = Device_config_GPIO_register_all_device();
  if (err) return err;
  
  err = Device_config_timer_register_all_device();
  if (err) return err;
  
  err = Device_config_PWM_register_all_device();
  if (err) return err;
  
  err = Device_config_SPI_register_all_device();
  if (err) return err;
  
  err = Device_config_ST7789V2_register_all_device();
  if (err) return err;
  
  err = Device_config_motor_register_all_device();
  if (err) return err;
  
  err = Device_config_speed_test_register_all_device();
  if (err) return err;

  return ESUCCESS;
}
