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
  Device_USART_name name;
  void *channel;
  const struct Device_USART_ops *ops;
} Device_USART;

typedef struct Device_USART_ops {
  errno_t (*init)(Device_USART *pd);
  errno_t (*transmit)(Device_USART *pd, uint8_t *data, uint32_t len);
  errno_t (*receive)(Device_USART *pd, uint8_t *data, uint32_t *data_len, uint32_t len);
} Device_USART_ops;

typedef struct Driver_USART_ops {
  errno_t (*receive)(Device_USART *pd, uint8_t *data, uint32_t len);
  errno_t (*transmit)(Device_USART *pd, uint8_t *data, uint32_t len);
  errno_t (*receive_IT)(Device_USART *pd, uint8_t *data, uint32_t len);
  errno_t (*transmit_IT)(Device_USART *pd, uint8_t *data, uint32_t len);
  errno_t (*abort_receive)(Device_USART *pd);
  errno_t (*abort_transmit)(Device_USART *pd);
} Driver_USART_ops;

errno_t Device_USART_module_init(void);
errno_t Device_USART_register(Device_USART *const pd);
errno_t Device_USART_find(Device_USART **pd_ptr, const Device_USART_name name);

errno_t Device_USART_TxCpltCallback(Device_USART *pd);
errno_t Device_USART_RxCpltCallback(Device_USART *pd);
