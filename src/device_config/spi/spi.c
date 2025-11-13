#include "spi.h"
#include "stm32f4xx_hal.h"
#include "device/spi/spi.h"
#include "driver/spi/spi.h"
#include "Core/Inc/spi.h"

static Device_SPI devices[DEVICE_SPI_COUNT] = {
  [DEVICE_SPI_1] = {
    .channel = &hspi1,
  },
};

errno_t Device_config_SPI_register_all_device(void) {
  for (Device_SPI_name name = 0; name < DEVICE_SPI_COUNT; ++name) {
    devices[name].name = name;
    errno_t err = Device_SPI_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi1) {
    Device_SPI_TxCpltCallback(&devices[DEVICE_SPI_1]);
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi1) {
    Device_SPI_RxCpltCallback(&devices[DEVICE_SPI_1]);
  }
}
