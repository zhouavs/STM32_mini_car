#include "gpio.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"

static errno_t read(const Device_GPIO *const pd, Pin_value *value_ptr);
static errno_t write(const Device_GPIO *const pd, const Pin_value value);

static const Driver_GPIO_ops ops = {
  .read = read,
  .write = write,
};

static errno_t read(const Device_GPIO *const pd, Pin_value *value_ptr) {
  if (pd == NULL) return EINVAL;
  *value_ptr = (Pin_value)HAL_GPIO_ReadPin((GPIO_TypeDef *)pd->port, pd->pin);
  return ESUCCESS;
}

static errno_t write(const Device_GPIO *const pd, const Pin_value value) {
  if (pd == NULL) return EINVAL;
  HAL_GPIO_WritePin((GPIO_TypeDef *)pd->port, pd->pin, (GPIO_PinState)value);
  return ESUCCESS;
}

errno_t Driver_GPIO_get_ops(const Driver_GPIO_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}
