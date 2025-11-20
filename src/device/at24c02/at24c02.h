#pragma once

#include <stdint.h>
#include "common/errno/errno.h"
#include "device/i2c/i2c.h"

typedef enum {
  DEVICE_AT24C02_1,
  DEVICE_AT24C02_COUNT,
} Device_AT24C02_name;

struct Device_AT24C02;
struct Device_AT24C02_ops;

typedef struct Device_AT24C02 {
  const Device_AT24C02_name name;
  const uint8_t addr;
  const uint8_t page_size;
  const uint8_t page_count;
  const uint16_t size;
  Device_I2C *i2c;
  const struct Device_AT24C02_ops *ops;
} Device_AT24C02;

typedef struct Device_AT24C02_ops {
  errno_t (*init)(const Device_AT24C02 *const pd);
  errno_t (*read)(const Device_AT24C02 *const pd, uint16_t addr, uint8_t *rt_data, uint16_t len);
  errno_t (*write)(const Device_AT24C02 *const pd, uint16_t addr, uint8_t *data, uint16_t len);
} Device_AT24C02_ops;

errno_t Device_AT24C02_module_init(void);
errno_t Device_AT24C02_register(Device_AT24C02 *const pd);
errno_t Device_AT24C02_find(const Device_AT24C02 **rt_pd_ptr, Device_AT24C02_name name);
