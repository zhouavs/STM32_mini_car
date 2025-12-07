#include "gpio.h"
#include "device/gpio/gpio.h"
#include "device_config/gpio/gpio.h"
#include "device_config/keyboard/keyboard.h"
#include "device_config/timer/timer.h"
#include "common/errno/errno.h"
#include <stdlib.h>
#include "common/delay/delay.h"

void gpio_test(void) {
  const Device_GPIO_name led_names[4] = { DEVICE_LED_1_OUT, DEVICE_LED_2_OUT, DEVICE_LED_3_OUT, DEVICE_LED_4_OUT };
  Device_GPIO *leds[sizeof(led_names) / sizeof(Device_GPIO_name)] = {0};
  const uint8_t led_count = sizeof(led_names) / sizeof(Device_GPIO_name);

  errno_t err = ESUCCESS;

  err = Device_config_GPIO_register_all_device();
  if (err) goto err_flag;

  err = Device_config_keyboard_register_all_device();
  if (err) goto err_flag;

  err = Device_config_timer_register_all_device();
  if (err) goto err_flag;

  for (uint8_t i = 0; i < led_count; ++i) {
    err = Device_GPIO_find(&leds[i], led_names[i]);
    if (err) goto err_flag;
  }

  const Device_keyboard *keyboard = NULL;
  err = Device_keyboard_find(&keyboard, DEVICE_KEYBOARD_1);
  if (err) goto err_flag;

  while (1) {
    Device_key_name key_name;
    err = keyboard->ops->read(&key_name);
    if (err) continue;

    switch (key_name) {
      case DEVICE_KEY_1: {
        err = leds[0]->ops->write(leds[0], PIN_VALUE_0);
        if (err) goto err_flag;
        break;
      }
      case DEVICE_KEY_2: {
        err = leds[1]->ops->write(leds[1], PIN_VALUE_0);
        if (err) goto err_flag;
        break;
      }
      case DEVICE_KEY_3: {
        err = leds[2]->ops->write(leds[2], PIN_VALUE_0);
        if (err) goto err_flag;
        break;
      }
      case DEVICE_KEY_4: {
        err = leds[3]->ops->write(leds[3], PIN_VALUE_0);
        if (err) goto err_flag;
        break;
      }
      default: break;
    }

    errno_t err = delay_ms(1000);
    if (err) goto err_flag;

    for (uint8_t i = 0; i < led_count; ++i) {
      leds[i]->ops->write(leds[i], PIN_VALUE_1);
    }
  }

  err_flag:
  for (;;);
}
