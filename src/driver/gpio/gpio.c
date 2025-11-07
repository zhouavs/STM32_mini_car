#include "gpio.h"
#include <stdlib.h>
// Use the HAL umbrella header to ensure all required types (e.g., HAL_StatusTypeDef)
// and module headers are brought in with the correct order via stm32f4xx_hal_conf.h
#include "stm32f4xx_hal.h"

static errno_t read(const Device_GPIO *const pd, Device_GPIO_value *value_ptr);
static errno_t write(const Device_GPIO *const pd, const Device_GPIO_value value);

static const Driver_GPIO_ops ops = {
  .read = read,
  .write = write,
};

static errno_t read(const Device_GPIO *const pd, Device_GPIO_value *value_ptr) {
  if (pd == NULL) return EINVAL;
  *value_ptr = (Device_GPIO_value)HAL_GPIO_ReadPin((GPIO_TypeDef *)pd->port, pd->pin);
  return ESUCCESS;
}

static errno_t write(const Device_GPIO *const pd, const Device_GPIO_value value) {
  if (pd == NULL) return EINVAL;
  HAL_GPIO_WritePin((GPIO_TypeDef *)pd->port, pd->pin, (GPIO_PinState)value);
  return ESUCCESS;
}

errno_t Driver_GPIO_get_ops(const Driver_GPIO_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  
}
