#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device/spi/spi.h"
#include <stdint.h>

typedef enum {
  DEVICE_W25Q64,
  DEVICE_W25QX_COUNT,
} Device_W25QX_name;

struct Device_W25QX;
struct Device_W25QX_ops;

typedef struct Device_W25QX {
  const Device_W25QX_name name;
  Device_GPIO *cs;
  Device_SPI *spi;
  const struct Device_W25QX_ops *ops;
} Device_W25QX;

typedef struct Device_W25QX_ops {
  errno_t (*init)(const Device_W25QX *const pd);
  errno_t (*erase)(const Device_W25QX *const pd, uint32_t addr, uint16_t sector_count);
  errno_t (*read)(const Device_W25QX *const pd, uint32_t addr, uint8_t *data, uint32_t len);
  errno_t (*write)(const Device_W25QX *const pd, uint32_t addr, uint8_t *data, uint32_t len);
} Device_W25QX_ops;

// 全局方法
errno_t Device_W25QX_module_init(void);
errno_t Device_W25QX_register(Device_W25QX *const pd);
errno_t Device_W25QX_find(const Device_W25QX **pd_ptr, const Device_W25QX_name name);
