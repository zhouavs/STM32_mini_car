#include "gpio.h"
#include "device/gpio/gpio.h"
#include "device_config/gpio/gpio.h"
#include "common/errno/errno.h"
#include <stdlib.h>

void gpio_test(void) {
  errno_t err = Device_config_GPIO_register_all_device();
  if (err) return;

  Device_GPIO *io_device = NULL;
  err = Device_GPIO_find(&io_device, LED_1);
  if (err) return;

  io_device->ops->Write(io_device, DEVICE_GPIO_PIN_SET);

  while (1) {
  
  }
}
