#pragma once

#include "common/errno/errno.h"
#include "device/i2c/i2c.h"

errno_t Driver_I2C_get_ops(const Driver_I2C_ops **po_ptr);
