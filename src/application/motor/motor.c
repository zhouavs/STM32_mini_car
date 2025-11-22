#include "motor.h"
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
#include "device/timer/timer.h"
#include "device_config/timer/timer.h"
#include "device/pwm/pwm.h"
#include "device_config/pwm/pwm.h"
#include "device/motor/motor.h"
#include "device_config/motor/motor.h"
#include "common/delay/delay.h"

static errno_t init(void);

void motor_test() {
  errno_t err = init();
  if (err) goto print_err_tag;

  const Device_motor_name names[4] = { DEVICE_MOTOR_HEAD_LEFT, DEVICE_MOTOR_HEAD_RIGHT, DEVICE_MOTOR_TAIL_LEFT, DEVICE_MOTOR_TAIL_RIGHT };
  Device_motor *pms[4] = {0};

  for (uint8_t i = 0; i < 4; ++i) {
    err = Device_motor_find(&pms[i], names[i]);
    if (err) goto print_err_tag;
    err = pms[i]->ops->init(pms[i]);
    if (err) goto print_err_tag;
  }

  uint8_t i = 0;
  while (1) {
    err = pms[i]->ops->set_speed(pms[i], 0x40);
    if (err) goto print_err_tag;
    err = pms[i]->ops->forward(pms[i]);
    if (err) goto print_err_tag;
    delay_s(2);
    err = pms[i]->ops->backward(pms[i]);
    if (err) goto print_err_tag;
    delay_s(2);
    err = pms[i]->ops->stop(pms[i]);
    if (err) goto print_err_tag;

    ++i;
    if (i == 4) i = 0;
  }

  print_err_tag:
  printf("motor_test_err\r\nerr: %d\r\n", err);
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

  err = Device_timer_module_init();
  if (err) goto print_err_tag;
  
  err = Device_config_timer_register_all_device();
  if (err) goto print_err_tag;
  
  err = Device_PWM_module_init();
  if (err) goto print_err_tag;

  err = Device_config_PWM_register_all_device();
  if (err) goto print_err_tag;
  
  err = Device_motor_module_init();
  if (err) goto print_err_tag;

  err = Device_config_motor_register_all_device();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("st7789v2_test_init_err\r\nerr: %d\r\n", err);
  return err;
}
