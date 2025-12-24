#include "servo.h"
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
#include "device/servo/servo.h"
#include "device_config/servo/servo.h"
#include "common/delay/delay.h"

static errno_t init(void);

void servo_test() {
  errno_t err = init();
  if (err) goto print_err_tag;

  Device_servo *pds = NULL;
  err = Device_servo_find(&pds, DEVICE_SERVO_1);
  if (err) goto print_err_tag;
  
  err = pds->ops->init(pds);
  if (err) goto print_err_tag;
  err = pds->ops->set_angle(pds, 0);
  if (err) goto print_err_tag;
  err = pds->ops->start(pds);
  if (err) goto print_err_tag;

  while (1) {

  }

  print_err_tag:
  printf("servo_test_err\r\nerr: %d\r\n", err);
  while (1);

  return;
}

static errno_t init(void) {
  errno_t err = ESUCCESS;

  err = Device_USART_module_init();
  if (err) return err;

  err = Device_config_USART_register();
  if (err) return err;

  err = Device_GPIO_module_init();
  if (err) goto print_err_tag;

  err = Device_config_GPIO_register();
  if (err) goto print_err_tag;

  err = Device_SPI_module_init();
  if (err) goto print_err_tag;

  err = Device_config_SPI_register();
  if (err) goto print_err_tag;

  err = Device_W25QX_module_init();
  if (err) goto print_err_tag;

  err = Device_config_W25QX_register();
  if (err) goto print_err_tag;

  err = Device_ST7789V2_module_init();
  if (err) goto print_err_tag;

  err = Device_config_ST7789V2_register();
  if (err) goto print_err_tag;

  err = Device_I2C_module_init();
  if (err) goto print_err_tag;

  err = Device_config_I2C_register();
  if (err) goto print_err_tag;

  err = Device_AT24C02_module_init();
  if (err) goto print_err_tag;

  err = Device_config_AT24C02_register();
  if (err) goto print_err_tag;

  err = Device_timer_module_init();
  if (err) goto print_err_tag;
  
  err = Device_config_timer_register();
  if (err) goto print_err_tag;
  
  err = Device_PWM_module_init();
  if (err) goto print_err_tag;

  err = Device_config_PWM_register();
  if (err) goto print_err_tag;
  
  err = Device_motor_module_init();
  if (err) goto print_err_tag;

  err = Device_config_motor_register();
  if (err) goto print_err_tag;
  
  err = Device_servo_module_init();
  if (err) goto print_err_tag;

  err = Device_config_servo_register();
  if (err) goto print_err_tag;

  return ESUCCESS;

  print_err_tag:
  printf("st7789v2_test_init_err\r\nerr: %d\r\n", err);
  return err;
}
