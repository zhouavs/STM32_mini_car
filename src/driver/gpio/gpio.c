#include "gpio.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"

static errno_t read(const Device_GPIO *const pd, Pin_value *value_ptr);
static errno_t write(const Device_GPIO *const pd, const Pin_value value);
static errno_t set_EXTI_handle(const Device_GPIO *const pd, Device_GPIO_EXTI_trigger trigger, void (*callback)(void));

static const Driver_GPIO_ops ops = {
  .read = read,
  .write = write,
  .set_EXTI_handle = set_EXTI_handle,
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

static errno_t set_EXTI_handle(const Device_GPIO *const pd, Device_GPIO_EXTI_trigger trigger, void (*callback)(void)) {
  if (pd == NULL || pd->exti_handle == NULL || callback == NULL) return EINVAL;

  GPIO_TypeDef *hgpio = (GPIO_TypeDef *)pd->port;

  EXTI_HandleTypeDef *hexti = (EXTI_HandleTypeDef *)pd->exti_handle;
  hexti->PendingCallback = callback;

  uint32_t GPIOSel = 0;
  if (hgpio == GPIOA) {
    GPIOSel = EXTI_GPIOA;
  } else if (hgpio == GPIOE) {
    GPIOSel = EXTI_GPIOE;
  } else if (hgpio == GPIOB) {
    GPIOSel = EXTI_GPIOB;
  } else {
    // 其余 GPIO 口暂时不支持
    return EINVAL;
  }

  uint32_t config_trigger = 0;
  switch (trigger) {
    case DEVICE_GPIO_EXTI_TRIGGER_RISING:
      config_trigger = EXTI_TRIGGER_RISING;
      break;
    case DEVICE_GPIO_EXTI_TRIGGER_FALLING:
      config_trigger = EXTI_TRIGGER_FALLING;
      break;
    case DEVICE_GPIO_EXTI_TRIGGER_RISING_FALLING:
      config_trigger = EXTI_TRIGGER_RISING_FALLING;
      break;
    default:
      return EINVAL;
  }

  EXTI_ConfigTypeDef config = {
    .GPIOSel = GPIOSel,
    .Line = hexti->Line,
    .Trigger = config_trigger,
    .Mode = EXTI_MODE_INTERRUPT,
  };

  HAL_StatusTypeDef status = HAL_EXTI_SetConfigLine(hexti, &config);
  return status == HAL_OK ? ESUCCESS : EINTR;
}

errno_t Driver_GPIO_get_ops(const Driver_GPIO_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}
