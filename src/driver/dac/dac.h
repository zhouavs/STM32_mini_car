#pragma once

#include "common/errno/errno.h"
#include "device/dac/dac.h"

errno_t Driver_DAC_get_ops(const Driver_DAC_ops **po_ptr);
