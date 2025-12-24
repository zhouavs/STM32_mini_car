#include "dac.h"
#include "stm32f4xx_hal.h"
#include "driver/dac/dac.h"
#include "Core/Inc/dac.h"

static Device_DAC devices[DEVICE_DAC_COUNT] = {
  [DEVICE_DAC_LIGHT] = {
    .name = DEVICE_DAC_LIGHT,
    .channel = DEVICE_DAC_CHANNEL_1,
    .instance = &hdac,
  },
};

static const Device_timer_name relate_timer[DEVICE_DAC_COUNT] = {
  [DEVICE_DAC_LIGHT] = DEVICE_TIMER_TIM4,
};

errno_t Device_config_DAC_register(void) {
  errno_t err = Device_DAC_module_init();
  if (err) return err;

  for (Device_DAC_name name = 0; name < DEVICE_DAC_COUNT; ++name) {
    err = Device_timer_find(&devices[name].timer, relate_timer[name]);
    if (err) return err;

    err = Device_DAC_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
