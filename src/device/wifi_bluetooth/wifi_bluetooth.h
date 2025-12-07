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

typedef enum {
  RECEIVE_MODE_PASSIVE = 0,
  RECEIVE_MODE_ACTIVE = 1,
} Device_wifi_bluetooth_socket_receive_mode;

typedef enum {
  WORK_MODE_NO_INIT_OR_CLOSE_WIFI = 0,
  WORK_MODE_STA = 1,
  WORK_MODE_AP = 2,
  WORK_MODE_AP_AND_STA = 3,
} Device_wifi_bluetooth_work_mode;

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
  errno_t (*join_wifi_ap)(Device_wifi_bluetooth *const pd, const uint8_t *const ssid, const uint8_t *const pwd);
  errno_t (*create_socket_connection)(
    Device_wifi_bluetooth *const pd
    , Device_wifi_bluetooth_socket_type type
    , uint8_t *remote_host
    , uint16_t port
  );
  errno_t (*delete_socket_connection)(Device_wifi_bluetooth *const pd, uint32_t port);
  errno_t (*socket_send)(Device_wifi_bluetooth *const pd, uint32_t port, uint8_t *const data_buf, uint32_t data_len);
  errno_t (*socket_read)(Device_wifi_bluetooth *const pd, uint32_t port, uint8_t *const rt_data_ptr, uint32_t *const rt_data_len_ptr, uint32_t data_size);
} Device_wifi_bluetooth_ops;

// 全局方法
errno_t Device_wifi_bluetooth_module_init(void);
errno_t Device_wifi_bluetooth_register(Device_wifi_bluetooth *const pd);
errno_t Device_wifi_bluetooth_find(Device_wifi_bluetooth **pd_ptr, const Device_wifi_bluetooth_name name);
