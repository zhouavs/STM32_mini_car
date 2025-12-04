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

typedef enum {
  UDP_SERVER = 1,
  UDP_CLIENT = 2,
  TCP_SERVER = 3,
  TCP_CLIENT = 4,
  TCP_SEED = 5,
  SSL_SERVER = 6,
  SSL_CLIENT = 7,
  SSL_SEED = 8,
} Device_wifi_bluetooth_socket_type;

typedef enum {
  DISCONNECTED = 0,
  CONNECTING = 1,
  CONNECTION_SUCCESSFUL = 3,
  CONNECTION_FAILED = 4,
  CONNECTION_DELETING = 127,
} Device_wifi_bluetooth_socket_status;

typedef struct {
  uint32_t connect_id;
  Device_wifi_bluetooth_socket_type type;
  uint8_t *remote_host;
  uint16_t port;
} Net_node;

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
