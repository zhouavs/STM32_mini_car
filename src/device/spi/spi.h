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
  Device_SPI_name name;
  void *channel;
  const struct Device_SPI_ops *ops;
} Device_SPI;

typedef struct Device_SPI_ops {
  errno_t (*init)(Device_SPI *pd);
  errno_t (*transmit)(Device_SPI *pd, uint8_t *data, uint32_t len);
  errno_t (*receive)(Device_SPI *pd, uint8_t *data, uint32_t *data_len, uint32_t len);
} Device_SPI_ops;

typedef struct Driver_SPI_ops {
  errno_t (*receive)(Device_SPI *pd, uint8_t *data, uint32_t len);
  errno_t (*transmit)(Device_SPI *pd, uint8_t *data, uint32_t len);
  errno_t (*receive_DMA)(Device_SPI *pd, uint8_t *data, uint32_t len);
  errno_t (*transmit_DMA)(Device_SPI *pd, uint8_t *data, uint32_t len);
  errno_t (*abort_receive)(Device_SPI *pd);
  errno_t (*abort_transmit)(Device_SPI *pd);
} Driver_SPI_ops;

errno_t Device_SPI_module_init(void);
errno_t Device_SPI_register(Device_SPI *const pd);
errno_t Device_SPI_find(Device_SPI **pd_ptr, const Device_SPI_name name);

errno_t Device_SPI_TxCpltCallback(Device_SPI *pd);
errno_t Device_SPI_RxCpltCallback(Device_SPI *pd);
