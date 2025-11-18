#pragma once

#include "common/errno/errno.h"
#include "device/gpio/gpio.h"
#include "device/spi/spi.h"
#include <stdint.h>

typedef enum {
  DEVICE_ST7789V2_1,
  DEVICE_ST7789V2_COUNT,
} Device_ST7789V2_name;

struct Device_ST7789V2;
struct Device_ST7789V2_ops;

typedef struct Device_ST7789V2 {
  const Device_ST7789V2_name name;
  const uint16_t screen_width;
  const uint16_t screen_height;
  Device_GPIO *rst;
  Device_GPIO *cs;
  Device_GPIO *dc;
  Device_GPIO *backlight;
  Device_SPI *spi;
  uint8_t one_pixel_byte_num;
  uint16_t window_width;
  uint16_t window_height;
  uint8_t *pixel_bytes;
  uint32_t pixel_bytes_size;
  const struct Device_ST7789V2_ops *ops;
} Device_ST7789V2;

typedef struct Device_ST7789V2_ops {
  errno_t (*init)(const Device_ST7789V2 *const pd);
  errno_t (*on)(const Device_ST7789V2 *const pd);
  errno_t (*off)(const Device_ST7789V2 *const pd);
  errno_t (*set_window)(Device_ST7789V2 *const pd, uint16_t start_y, uint16_t start_x, uint16_t end_y, uint16_t end_x);
  errno_t (*refresh_window)(const Device_ST7789V2 *const pd);
  errno_t (*set_pixel)(const Device_ST7789V2 *const pd, uint16_t y, uint16_t x, uint16_t color);
} Device_ST7789V2_ops;

// 全局方法
errno_t Device_ST7789V2_module_init(void);
errno_t Device_ST7789V2_register(Device_ST7789V2 *const pd);
errno_t Device_ST7789V2_find(Device_ST7789V2 **pd_ptr, const Device_ST7789V2_name name);
