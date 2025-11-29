#pragma once

#include "common/errno/errno.h"
#include "device/rtc/rtc.h"

errno_t Driver_RTC_get_ops(const Driver_RTC_ops **po_ptr);
