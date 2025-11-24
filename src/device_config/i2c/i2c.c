#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "driver/i2c/i2c.h"
#include "Core/Inc/i2c.h"

static Device_I2C devices[DEVICE_I2C_COUNT] = {
  [DEVICE_I2C_1] = {
    .name = DEVICE_I2C_1,
    .instance = &hi2c1,
  },
};

errno_t Device_config_I2C_register_all_device(void) {
  errno_t err = Device_I2C_module_init();
  if (err) return err;

  for (Device_I2C_name name = 0; name < DEVICE_I2C_COUNT; ++name) {
    err = Device_I2C_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c == &hi2c1) {
    Device_I2C_MasterTxCpltCallback(&devices[DEVICE_I2C_1]);
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c == &hi2c1) {
    Device_I2C_MasterRxCpltCallback(&devices[DEVICE_I2C_1]);
  }
}
