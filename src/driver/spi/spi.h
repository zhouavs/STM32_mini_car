#pragma once

#include "common/errno/errno.h"
#include "device/spi/spi.h"

errno_t Driver_SPI_get_ops(const Driver_SPI_ops **po_ptr);
