#include "driver/gpio/gpio.h"
#include "stm32f4xx_hal.h"
#include "Core/Inc/stm32f4xx_it.h"
#include <stdint.h>
#include "device/keyboard/keyboard.h"

static Device_GPIO devices[DEVICE_GPIO_COUNT] = {
  [DEVICE_LED_1_OUT] = {
    .name = DEVICE_LED_1_OUT,
    .port = GPIOB,
    .pin = GPIO_PIN_12,
  },
  [DEVICE_LED_2_OUT] = {
    .name = DEVICE_LED_2_OUT,
    .port = GPIOB,
    .pin = GPIO_PIN_13,
  },
  [DEVICE_LED_3_OUT] = {
    .name = DEVICE_LED_3_OUT,
    .port = GPIOB,
    .pin = GPIO_PIN_14,
  },
  [DEVICE_LED_4_OUT] = {
    .name = DEVICE_LED_4_OUT,
    .port = GPIOB,
    .pin = GPIO_PIN_15,
  },
  [DEVICE_KEY_1_IN] = {
    .name = DEVICE_KEY_1_IN,
    .port = GPIOA,
    .pin = GPIO_PIN_0,
    .exti_handle = &hexti0,
  },
  [DEVICE_KEY_2_IN] = {
    .name = DEVICE_KEY_2_IN,
    .port = GPIOA,
    .pin = GPIO_PIN_1,
    .exti_handle = &hexti1,
  },
  [DEVICE_KEY_3_IN] = {
    .name = DEVICE_KEY_3_IN,
    .port = GPIOA,
    .pin = GPIO_PIN_2,
    .exti_handle = &hexti2,
  },
  [DEVICE_KEY_4_IN] = {
    .name = DEVICE_KEY_4_IN,
    .port = GPIOA,
    .pin = GPIO_PIN_3,
    .exti_handle = &hexti3,
  },
  [DEVICE_W25Q64_CS] = {
    .name = DEVICE_W25Q64_CS,
    .port = GPIOA,
    .pin = GPIO_PIN_15,
  },
  [DEVICE_ST7789V2_1_CS] = {
    .name = DEVICE_ST7789V2_1_CS,
    .port = GPIOD,
    .pin = GPIO_PIN_3,
  },
  [DEVICE_ST7789V2_1_RST] = {
    .name = DEVICE_ST7789V2_1_RST,
    .port = GPIOG,
    .pin = GPIO_PIN_15,
  },
  [DEVICE_ST7789V2_1_DC] = {
    .name = DEVICE_ST7789V2_1_DC,
    .port = GPIOF,
    .pin = GPIO_PIN_9,
  },
  [DEVICE_ST7789V2_1_BACKLIGHT] = {
    .name = DEVICE_ST7789V2_1_BACKLIGHT,
    .port = GPIOE,
    .pin = GPIO_PIN_6,
  },
  [DEVICE_MOTOR_HEAD_LEFT_IN_1] = {
    .name = DEVICE_MOTOR_HEAD_LEFT_IN_1,
    .port = GPIOG,
    .pin = GPIO_PIN_6,
  },
  [DEVICE_MOTOR_HEAD_LEFT_IN_2] = {
    .name = DEVICE_MOTOR_HEAD_LEFT_IN_2,
    .port = GPIOG,
    .pin = GPIO_PIN_7,
  },
  [DEVICE_MOTOR_HEAD_RIGHT_IN_1] = {
    .name = DEVICE_MOTOR_HEAD_RIGHT_IN_1,
    .port = GPIOG,
    .pin = GPIO_PIN_8,
  },
  [DEVICE_MOTOR_HEAD_RIGHT_IN_2] = {
    .name = DEVICE_MOTOR_HEAD_RIGHT_IN_2,
    .port = GPIOG,
    .pin = GPIO_PIN_9,
  },
  [DEVICE_MOTOR_TAIL_LEFT_IN_1] = {
    .name = DEVICE_MOTOR_TAIL_LEFT_IN_1,
    .port = GPIOG,
    .pin = GPIO_PIN_11,
  },
  [DEVICE_MOTOR_TAIL_LEFT_IN_2] = {
    .name = DEVICE_MOTOR_TAIL_LEFT_IN_2,
    .port = GPIOG,
    .pin = GPIO_PIN_12,
  },
  [DEVICE_MOTOR_TAIL_RIGHT_IN_1] = {
    .name = DEVICE_MOTOR_TAIL_RIGHT_IN_1,
    .port = GPIOG,
    .pin = GPIO_PIN_13,
  },
  [DEVICE_MOTOR_TAIL_RIGHT_IN_2] = {
    .name = DEVICE_MOTOR_TAIL_RIGHT_IN_2,
    .port = GPIOG,
    .pin = GPIO_PIN_14,
  },
  [DEVICE_SPEED_TEST_HEAD_LEFT_IN] = {
    .name = DEVICE_SPEED_TEST_HEAD_LEFT_IN,
    .port = GPIOE,
    .pin = GPIO_PIN_2,
    .exti_handle = &hexti2,
  },
  [DEVICE_SPEED_TEST_HEAD_RIGHT_IN] = {
    .name = DEVICE_SPEED_TEST_HEAD_RIGHT_IN,
    .port = GPIOE,
    .pin = GPIO_PIN_3,
    .exti_handle = &hexti3,
  },
  [DEVICE_SPEED_TEST_TAIL_LEFT_IN] = {
    .name = DEVICE_SPEED_TEST_TAIL_LEFT_IN,
    .port = GPIOE,
    .pin = GPIO_PIN_4,
    .exti_handle = &hexti4,
  },
  [DEVICE_SPEED_TEST_TAIL_RIGHT_IN] = {
    .name = DEVICE_SPEED_TEST_TAIL_RIGHT_IN,
    .port = GPIOE,
    .pin = GPIO_PIN_5,
    .exti_handle = &hexti5,
  },
  [DEVICE_TRACKER_IN_1] = {
    .name = DEVICE_TRACKER_IN_1,
    .port = GPIOC,
    .pin = GPIO_PIN_2,
  }, 
  [DEVICE_TRACKER_IN_2] = {
    .name = DEVICE_TRACKER_IN_2,
    .port = GPIOC,
    .pin = GPIO_PIN_3,
  }, 
  [DEVICE_TRACKER_IN_3] = {
    .name = DEVICE_TRACKER_IN_3,
    .port = GPIOC,
    .pin = GPIO_PIN_4,
  }, 
  [DEVICE_TRACKER_IN_4] = {
    .name = DEVICE_TRACKER_IN_4,
    .port = GPIOC,
    .pin = GPIO_PIN_5,
  }, 
  [DEVICE_TRACKER_IN_5] = {
    .name = DEVICE_TRACKER_IN_5,
    .port = GPIOF,
    .pin = GPIO_PIN_6,
  }, 
  [DEVICE_TRACKER_IN_6] = {
    .name = DEVICE_TRACKER_IN_6,
    .port = GPIOF,
    .pin = GPIO_PIN_7,
  }, 
  [DEVICE_TRACKER_IN_7] = {
    .name = DEVICE_TRACKER_IN_7,
    .port = GPIOF,
    .pin = GPIO_PIN_8,
  },
  [DEVICE_IRDA_IN] = {
    .name = DEVICE_IRDA_IN,
    .port = GPIOB,
    .pin = GPIO_PIN_1,
    .exti_handle = &hexti1,
  },
  [DEVICE_DHT11_IN] = {
    .name = DEVICE_DHT11_IN,
    .port = GPIOB,
    .pin = GPIO_PIN_5,
  },
  [DEVICE_ULTRASONIC_TRIG] = {
    .name = DEVICE_ULTRASONIC_TRIG,
    .port = GPIOD,
    .pin = GPIO_PIN_6,
  },
  [DEVICE_ULTRASONIC_ECHO] = {
    .name = DEVICE_ULTRASONIC_ECHO,
    .port = GPIOD,
    .pin = GPIO_PIN_7,
  },
};

errno_t Device_config_GPIO_register_all_device(void) {
  errno_t err = Device_GPIO_module_init();
  if (err) return err;
  
  for (Device_GPIO_name name = 0; name < DEVICE_GPIO_COUNT; ++name) {
    err = Device_GPIO_register(&devices[name]);
    if (err) return err;
  }

  return ESUCCESS;
}

// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
//   switch (GPIO_Pin) {
//     case GPIO_PIN_0: {
//       Device_GPIO_EXTI_callback(&devices[DEVICE_KEY_1_IN]);
//       Device_keyboard_EXTI_callback(&devices[DEVICE_KEY_1_IN]);
//       break;
//     }
//     case GPIO_PIN_1: {
//       Device_GPIO_EXTI_callback(&devices[DEVICE_KEY_2_IN]);
//       Device_keyboard_EXTI_callback(&devices[DEVICE_KEY_2_IN]);
//       break;
//     }
//     case GPIO_PIN_2: {
//       Device_GPIO_EXTI_callback(&devices[DEVICE_KEY_3_IN]);
//       Device_keyboard_EXTI_callback(&devices[DEVICE_KEY_3_IN]);
//       break;
//     }
//     case GPIO_PIN_3: {
//       Device_GPIO_EXTI_callback(&devices[DEVICE_KEY_4_IN]);
//       Device_keyboard_EXTI_callback(&devices[DEVICE_KEY_4_IN]);
//       break;
//     }
//     default: {
//       break;
//     }
//   }
// }
