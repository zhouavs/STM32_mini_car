#include "key.h"
#include <stdlib.h>
#include <stdint.h>
#include "device/gpio/gpio.h"
#include "device_config/gpio/gpio.h"

static void callback(void) {
  Device_GPIO *pk = NULL, *pd = NULL;
  errno_t err = Device_GPIO_find(&pk, DEVICE_KEY_1_IN);
  if (err) return;
  err = Device_GPIO_find(&pd, DEVICE_LED_1_OUT);
  if (err) return;

  Pin_value kv = PIN_VALUE_1;
  err = pk->ops->read(pk, &kv);
  if (err) return;
  err = pd->ops->write(pd, kv);
  if (err) return;
}

void key_test(void) {
  errno_t err = Device_GPIO_module_init();
  if (err) return;

  err = Device_config_GPIO_register();
  if (err) return;

  Device_GPIO *pk = NULL;
  err = Device_GPIO_find(&pk, DEVICE_KEY_1_IN);
  if (err) return;

  err = pk->ops->init(pk);
  if (err) return;
  
  err = pk->ops->set_EXTI_handle(pk, DEVICE_GPIO_EXTI_TRIGGER_RISING_FALLING, callback);
  if (err) return;

  while (1) {
  
  }
}
