#pragma once

#include "common/errno/errno.h"
#include <stdint.h>

typedef enum {
  DEVICE_SPI_1,
  DEVICE_SPI_COUNT,
} Device_SPI_name;

struct Device_SPI;
struct Device_SPI_ops;

typedef struct Device_SPI {
  const Device_SPI_name name;
  void *const channel;
  const struct Device_SPI_ops *ops;
} Device_SPI;

typedef struct Device_SPI_ops {
  errno_t (*init)(const Device_SPI *const pd);
  errno_t (*transmit)(const Device_SPI *const pd, const uint8_t *const data, uint32_t len);
  errno_t (*receive)(const Device_SPI *const pd, uint8_t *data, uint32_t len);
} Device_SPI_ops;

typedef struct Driver_SPI_ops {
  errno_t (*receive)(const Device_SPI *const pd, uint8_t *data, uint16_t len);
  errno_t (*transmit)(const Device_SPI *const pd, const uint8_t *const data, uint16_t len);
  errno_t (*receive_IT)(const Device_SPI *const pd, uint8_t *data, uint16_t len);
  errno_t (*transmit_IT)(const Device_SPI *const pd, const uint8_t *const data, uint16_t len);
  errno_t (*receive_DMA)(const Device_SPI *const pd, uint8_t *data, uint16_t len);
  errno_t (*transmit_DMA)(const Device_SPI *const pd, const uint8_t *const data, uint16_t len);
} Driver_SPI_ops;

errno_t Device_SPI_module_init(void);
errno_t Device_SPI_register(Device_SPI *const pd);
errno_t Device_SPI_find(Device_SPI **pd_ptr, const Device_SPI_name name);

errno_t Device_SPI_TxCpltCallback(const Device_SPI *const pd);
errno_t Device_SPI_RxCpltCallback(const Device_SPI *const pd);
