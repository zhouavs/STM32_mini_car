#include "gpio.h"
#include "device/gpio/gpio.h"
#include "device_config/gpio/gpio.h"
#include "common/errno/errno.h"
#include <stdlib.h>

void gpio_test(void) {
  errno_t err = Device_config_GPIO_register_all_device();
  if (err) return;

  Device_GPIO *led_1 = NULL, *key_1 = NULL;
  err = Device_GPIO_find(&led_1, LED_1);
  if (err) return;

  err = Device_GPIO_find(&key_1, KEY_1);
  if (err) return;

  led_1->ops->write(led_1, DEVICE_GPIO_PIN_SET);

  while (1) {
    Device_GPIO_value key_1_state = DEVICE_GPIO_PIN_SET;
    err = key_1->ops->read(key_1, &key_1_state);
    if (err) return;
    if (key_1_state == DEVICE_GPIO_PIN_RESET) {
      led_1->ops->write(led_1, DEVICE_GPIO_PIN_RESET);
    } else {
      led_1->ops->write(led_1, DEVICE_GPIO_PIN_SET);
    }
  }
}
