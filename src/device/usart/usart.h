#pragma once

#include "common/errno/errno.h"
#include <stdint.h>

typedef enum {
  DEVICE_USART_DEBUG,
  DEVICE_USART_COUNT,
} Device_USART_name;

struct Device_USART;
struct Device_USART_ops;

typedef struct Device_USART {
  const Device_USART_name name;
  void *const instance;
  const struct Device_USART_ops *ops;
} Device_USART;

typedef struct Device_USART_ops {
  errno_t (*init)(const Device_USART *const pd);
  errno_t (*transmit)(const Device_USART *const pd, uint8_t *data, uint32_t len);
  errno_t (*receive)(const Device_USART *const pd, uint8_t *data, uint32_t *data_len, uint32_t len);
} Device_USART_ops;

typedef struct Driver_USART_ops {
  errno_t (*receive)(const Device_USART *const pd, uint8_t *data, uint32_t len);
  errno_t (*transmit)(const Device_USART *const pd, uint8_t *data, uint32_t len);
  errno_t (*receive_IT)(const Device_USART *const pd, uint8_t *data, uint32_t len);
  errno_t (*transmit_IT)(const Device_USART *const pd, uint8_t *data, uint32_t len);
  errno_t (*abort_receive)(const Device_USART *const pd);
  errno_t (*abort_transmit)(const Device_USART *const pd);
} Driver_USART_ops;

errno_t Device_USART_module_init(void);
errno_t Device_USART_register(Device_USART *const pd);
errno_t Device_USART_find(Device_USART **pd_ptr, const Device_USART_name name);

errno_t Device_USART_TxCpltCallback(const Device_USART *const pd);
errno_t Device_USART_RxCpltCallback(const Device_USART *const pd);
