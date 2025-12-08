#pragma once

#include "common/errno/errno.h"
#include "device/adc/adc.h"

errno_t Driver_ADC_get_ops(const Driver_ADC_ops **po_ptr);
