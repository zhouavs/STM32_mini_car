#pragma once

#include "common/errno/errno.h"
#include "device/usart/usart.h"

errno_t Driver_USART_get_ops(const Driver_USART_ops **po_ptr);
