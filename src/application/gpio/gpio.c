#include "gpio.h"
#include "device/gpio/gpio.h"
#include "device_config/gpio/gpio.h"
#include "device/keyboard/keyboard.h"
#include "common/errno/errno.h"
#include <stdlib.h>
#include "common/delay/delay.h"

void gpio_test(void) {
  const Device_GPIO_name led_names[4] = { DEVICE_LED_1, DEVICE_LED_2, DEVICE_LED_3, DEVICE_LED_4 };
  Device_GPIO *leds[sizeof(led_names) / sizeof(Device_GPIO_name)] = {0};
  const uint8_t led_count = sizeof(led_names) / sizeof(Device_GPIO_name);

  errno_t err = ESUCCESS;

  err = Device_GPIO_module_init();
  if (err) return;
  err = Device_keyboard_module_init();
  if (err) return;
  err = Device_config_GPIO_register_all_device();
  if (err) return;

  for (uint8_t i = 0; i < led_count; ++i) {
    err = Device_GPIO_find(&leds[i], led_names[i]);
    if (err) return;
  }

  const Device_keyboard *keyboard = NULL;
  err = Device_keyboard_get_device(&keyboard);
  if (err) return;

  while (1) {
    Device_GPIO_name key_name;
    err = keyboard->ops->read(&key_name);
    if (err) continue;

    switch (key_name) {
      case DEVICE_KEY_1: {
        leds[0]->ops->write(leds[0], DEVICE_GPIO_PIN_RESET);
        break;
      }
      case DEVICE_KEY_2: {
        leds[1]->ops->write(leds[1], DEVICE_GPIO_PIN_RESET);
        break;
      }
      case DEVICE_KEY_3: {
        leds[2]->ops->write(leds[2], DEVICE_GPIO_PIN_RESET);
        break;
      }
      case DEVICE_KEY_4: {
        leds[3]->ops->write(leds[3], DEVICE_GPIO_PIN_RESET);
        break;
      }
      default: break;
    }

    delay_ms(1000);

    for (uint8_t i = 0; i < led_count; ++i) {
      leds[i]->ops->write(leds[i], DEVICE_GPIO_PIN_SET);
    }
  }
}
