#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"

errno_t Driver_GPIO_get_ops(const Driver_GPIO_ops **po_ptr);
