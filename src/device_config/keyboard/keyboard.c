#include "keyboard.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdlib.h>

static void key_1_callback(void);
static void key_2_callback(void);
static void key_3_callback(void);
static void key_4_callback(void);

static Device_keyboard devices[DEVICE_KEYBOARD_COUNT] = {
  [DEVICE_KEYBOARD_1] = {
    .name = DEVICE_KEYBOARD_1,
    .keys = {
      DEVICE_KEY_1,
      DEVICE_KEY_2,
      DEVICE_KEY_3,
      DEVICE_KEY_4,
    },
  },
};

static const Device_GPIO_name relate_ins[DEVICE_KEY_COUNT] = {
  [DEVICE_KEY_1] = DEVICE_KEY_1_IN,
  [DEVICE_KEY_2] = DEVICE_KEY_2_IN,
  [DEVICE_KEY_3] = DEVICE_KEY_3_IN,
  [DEVICE_KEY_4] = DEVICE_KEY_4_IN,
};

static void (*const callbacks[DEVICE_KEY_COUNT])(void) = {
  [DEVICE_KEY_1] = key_1_callback,
  [DEVICE_KEY_2] = key_2_callback,
  [DEVICE_KEY_3] = key_3_callback,
  [DEVICE_KEY_4] = key_4_callback,
};

errno_t Device_config_keyboard_register(void) {
  errno_t err = Device_keyboard_module_init();
  if (err) return err;
  
  for (Device_keyboard_name name = 0; name < DEVICE_KEYBOARD_COUNT; ++name) {
    for (Device_key_name kn = 0; kn < DEVICE_KEY_COUNT; ++kn) {
      Device_GPIO *in = NULL;
      err = Device_GPIO_find(&in, relate_ins[kn]);
      if (err) return err;

      devices[name].ins[kn] = in;

      err = in->ops->set_EXTI_handle(in, DEVICE_GPIO_EXTI_TRIGGER_FALLING, callbacks[kn]);
      if (err) return err;
    }

    err = Device_keyboard_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

static void key_1_callback(void) {
  const Device_keyboard *pd = &devices[DEVICE_KEYBOARD_1];
  Device_keyboard_in_EXTI_callback(pd, DEVICE_KEY_1);
}

static void key_2_callback(void) {
  const Device_keyboard *pd = &devices[DEVICE_KEYBOARD_1];
  Device_keyboard_in_EXTI_callback(pd, DEVICE_KEY_2);
}

static void key_3_callback(void) {
  const Device_keyboard *pd = &devices[DEVICE_KEYBOARD_1];
  Device_keyboard_in_EXTI_callback(pd, DEVICE_KEY_3);
}

static void key_4_callback(void) {
  const Device_keyboard *pd = &devices[DEVICE_KEYBOARD_1];
  Device_keyboard_in_EXTI_callback(pd, DEVICE_KEY_4);
}
