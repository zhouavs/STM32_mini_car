#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  DEVICE_DHT11_1,
  DEVICE_DHT11_COUNT,
} Device_DHT11_name;

struct Device_DHT11;
struct Device_DHT11_ops;

typedef struct Device_DHT11 {
  const Device_DHT11_name name;
  Device_GPIO *in;
  Device_timer *timer;
  const struct Device_DHT11_ops *ops;
} Device_DHT11;

typedef struct Device_DHT11_ops {
  errno_t (*init)(Device_DHT11 *const pd);
  errno_t (*read)(Device_DHT11 *const pd, uint8_t rt_data[4]);
} Device_DHT11_ops;

// 全局方法
errno_t Device_DHT11_module_init(void);
errno_t Device_DHT11_register(Device_DHT11 *const pd);
errno_t Device_DHT11_find(Device_DHT11 **pd_ptr, const Device_DHT11_name name);
