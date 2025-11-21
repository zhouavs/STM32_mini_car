#pragma once

#include "common/errno/errno.h"
#include <stdint.h>

typedef enum {
  DEVICE_I2C_1,
  DEVICE_I2C_COUNT,
} Device_I2C_name;

struct Device_I2C;
struct Device_I2C_ops;

typedef struct Device_I2C {
  const Device_I2C_name name;
  void *const channel;
  uint16_t own_addr;
  const struct Device_I2C_ops *ops;
} Device_I2C;

typedef struct Device_I2C_ops {
  errno_t (*init)(Device_I2C *const pd);
  errno_t (*is_device_ready)(const Device_I2C *const pd, uint16_t slave_addr, uint32_t trial_num, uint32_t timeout);
  errno_t (*transmit)(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *const data, uint32_t len);
  errno_t (*receive)(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *rt_data, uint32_t len);
} Device_I2C_ops;

typedef struct Driver_I2C_ops {
  errno_t (*get_own_addr)(const Device_I2C *const pd, uint16_t *rt_own_addr_ptr);
  errno_t (*is_device_ready)(const Device_I2C *const pd, uint16_t slave_addr, uint32_t trial_num, uint32_t timeout);
  errno_t (*master_receive)(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *rt_data, uint16_t len);
  errno_t (*master_transmit)(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *const data, uint16_t len);
  errno_t (*master_receive_IT)(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *rt_data, uint16_t len);
  errno_t (*master_transmit_IT)(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *const data, uint16_t len);
  errno_t (*master_receive_DMA)(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *rt_data, uint16_t len);
  errno_t (*master_transmit_DMA)(const Device_I2C *const pd, uint16_t slave_addr, uint8_t *const data, uint16_t len);
} Driver_I2C_ops;

errno_t Device_I2C_module_init(void);
errno_t Device_I2C_register(Device_I2C *const pd);
errno_t Device_I2C_find(Device_I2C **pd_ptr, const Device_I2C_name name);

errno_t Device_I2C_TxCpltCallback(const Device_I2C *const pd);
errno_t Device_I2C_RxCpltCallback(const Device_I2C *const pd);
