#include "adc.h"
#include "stm32f4xx_hal.h"
#include "driver/adc/adc.h"
#include "Core/Inc/adc.h"

static Device_ADC devices[DEVICE_ADC_COUNT] = {
  [DEVICE_ADC_POWER] = {
    .name = DEVICE_ADC_POWER,
    .instance = &hadc1,
    .channel = DEVICE_ADC_CHANNEL_10,
    .sampling_time = DEVICE_ADC_SAMPLING_TIME_144_CYCLES,
  },
  [DEVICE_ADC_LIGHT] = {
    .name = DEVICE_ADC_LIGHT,
    .instance = &hadc1,
    .channel = DEVICE_ADC_CHANNEL_11,
    .sampling_time = DEVICE_ADC_SAMPLING_TIME_144_CYCLES,
  },
};

errno_t Device_config_ADC_register(void) {
  errno_t err = Device_ADC_module_init();
  if (err) return err;

  for (Device_ADC_name name = 0; name < DEVICE_ADC_COUNT; ++name) {
    err = Device_ADC_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}
