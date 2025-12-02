#pragma once

#include "common/errno/errno.h"
#include "device/usart/usart.h"
#include "device/timer/timer.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  DEVICE_WIFI_BLUETOOTH_1,
  DEVICE_WIFI_BLUETOOTH_COUNT,
} Device_wifi_bluetooth_name;

struct Device_wifi_bluetooth;
struct Device_wifi_bluetooth_ops;

typedef struct Device_wifi_bluetooth {
  const Device_wifi_bluetooth_name name;
  Device_USART *usart;
  Device_timer *timer;
  const struct Device_wifi_bluetooth_ops *ops;
} Device_wifi_bluetooth;

typedef struct Device_wifi_bluetooth_ops {
  errno_t (*init)(Device_wifi_bluetooth *const pd);
} Device_wifi_bluetooth_ops;

// 全局方法
errno_t Device_wifi_bluetooth_module_init(void);
errno_t Device_wifi_bluetooth_register(Device_wifi_bluetooth *const pd);
errno_t Device_wifi_bluetooth_find(Device_wifi_bluetooth **pd_ptr, const Device_wifi_bluetooth_name name);
