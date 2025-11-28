#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device/timer/timer.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  DEVICE_IRDA_CMD_NONE = 0x00,
  DEVICE_IRDA_CMD_Open = 0x45, DEVICE_IRDA_CMD_Menu = 0x47, DEVICE_IRDA_CMD_Test = 0x44, DEVICE_IRDA_CMD_ADD = 0x40, DEVICE_IRDA_CMD_Return = 0x43,
  DEVICE_IRDA_CMD_Back = 0x07, DEVICE_IRDA_CMD_Suspend = 0x15, DEVICE_IRDA_CMD_Forward = 0x09, DEVICE_IRDA_CMD_Num_0 = 0x16, DEVICE_IRDA_CMD_SUB = 0x19,
  DEVICE_IRDA_CMD_Cancle = 0x0D, DEVICE_IRDA_CMD_NUM_1 = 0x0C, DEVICE_IRDA_CMD_NUM_2 = 0x18, DEVICE_IRDA_CMD_NUM_3 = 0x5E, DEVICE_IRDA_CMD_NUM_4 = 0x08,
  DEVICE_IRDA_CMD_NUM_5 = 0x1C, DEVICE_IRDA_CMD_NUM_6 = 0x5A, DEVICE_IRDA_CMD_NUM_7 = 0x42, DEVICE_IRDA_CMD_NUM_8 = 0x52, DEVICE_IRDA_CMD_NUM_9 = 0x4A,
} Device_IRDA_cmd;

typedef enum {
  DEVICE_IRDA_1,
  DEVICE_IRDA_COUNT,
} Device_IRDA_name;

struct Device_IRDA;
struct Device_IRDA_ops;

typedef struct Device_IRDA {
  const Device_IRDA_name name;
  Device_GPIO *in;
  Device_timer *timer;
  const struct Device_IRDA_ops *ops;
} Device_IRDA;

typedef struct Device_IRDA_ops {
  errno_t (*init)(Device_IRDA *const pd);
  errno_t (*read)(Device_IRDA *const pd, Device_IRDA_cmd *rt_cmd_ptr);
} Device_IRDA_ops;

// 全局方法
errno_t Device_IRDA_module_init(void);
errno_t Device_IRDA_register(Device_IRDA *const pd);
errno_t Device_IRDA_find(Device_IRDA **pd_ptr, const Device_IRDA_name name);

// 红外接收探头外部中断回调处理
errno_t Device_IRDA_in_EXTI_callback(Device_IRDA *const pd);
