#include "st7789v2.h"
#include "cmd.h"
#include "common/delay/delay.h"
#include "common/list/list.h"
#include "font/index.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// 对象方法
static errno_t init(const Device_ST7789V2 *const pd);
static errno_t on(const Device_ST7789V2 *const pd);
static errno_t off(const Device_ST7789V2 *const pd);
static errno_t set_display_memory(Device_ST7789V2 *const pd, uint8_t *memory_ptr, uint32_t memory_size);
static errno_t set_window(Device_ST7789V2 *const pd, uint16_t start_y, uint16_t start_x, uint16_t end_y, uint16_t end_x);
static errno_t set_pixel(const Device_ST7789V2 *const pd, uint16_t y, uint16_t x, color_t color);
static errno_t set_ascii_char(Device_ST7789V2 *pds, uint8_t ch, uint16_t start_y, uint16_t start_x, uint16_t color);
static errno_t set_ascii_str(Device_ST7789V2 *pds, const uint8_t *const str, uint32_t len, uint16_t start_y, uint16_t start_x, color_t color);
static errno_t fill_window(Device_ST7789V2 *const pd, color_t color);
static errno_t refresh_window(const Device_ST7789V2 *const pd);
static errno_t clear_screen(Device_ST7789V2 *const pd, color_t color);

// 内部方法
// 设备命令方法
static errno_t hardware_reset(const Device_ST7789V2 *const pd);
static errno_t no_operation(const Device_ST7789V2 *const pd) __attribute__((unused));
static errno_t software_reset(const Device_ST7789V2 *const pd) __attribute__((unused));
static errno_t read_display_id(const Device_ST7789V2 *const pd, uint8_t rt_ids[3]) __attribute__((unused));
static errno_t read_display_status(const Device_ST7789V2 *const pd, uint8_t rt_status[4]) __attribute__((unused));
static errno_t sleep_in(const Device_ST7789V2 *const pd) __attribute__((unused));
static errno_t sleep_out(const Device_ST7789V2 *const pd);
static errno_t partial_mode_on(const Device_ST7789V2 *const pd) __attribute__((unused));
static errno_t normal_mode_on(const Device_ST7789V2 *const pd);
static errno_t display_inversion_off(const Device_ST7789V2 *const pd);
static errno_t display_inversion_on(const Device_ST7789V2 *const pd) __attribute__((unused));
static errno_t display_off(const Device_ST7789V2 *const pd);
static errno_t display_on(const Device_ST7789V2 *const pd);
static errno_t col_addr_set(const Device_ST7789V2 *const pd, uint16_t start, uint16_t end);
static errno_t row_addr_set(const Device_ST7789V2 *const pd, uint16_t start, uint16_t end);
static errno_t memory_write(const Device_ST7789V2 *const pd, uint8_t *data, uint32_t len);
static errno_t memory_data_access_control(const Device_ST7789V2 *const pd, uint8_t param);
static errno_t set_pixel_color_format(const Device_ST7789V2 *const pd, uint8_t bit_num);
// 简化操作
static errno_t write_register(const Device_ST7789V2 *const pd, const uint8_t cmd);
static errno_t write_data(const Device_ST7789V2 *const pd, const uint8_t *data, uint32_t len);
static errno_t read_data(const Device_ST7789V2 *const pd, uint8_t *rt_data, uint32_t len);
// 检查对象是否完整
static inline uint8_t pd_is_cplt(const Device_ST7789V2 *const pd);
// 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

// 全局变量
static List *list = NULL;
static const Device_ST7789V2_ops device_ops = {
  .init = init,
  .on = on,
  .off = off,
  .set_display_memory = set_display_memory,
  .set_window = set_window,
  .set_pixel = set_pixel,
  .set_ascii_char = set_ascii_char,
  .set_ascii_str = set_ascii_str,
  .fill_window = fill_window,
  .refresh_window = refresh_window,
  .clear_screen = clear_screen,
};

errno_t Device_ST7789V2_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_ST7789V2_register(Device_ST7789V2 *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_ST7789V2_find(Device_ST7789V2 **pd_ptr, const Device_ST7789V2_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}


static errno_t init(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->init(pd->cs);
  if (err) return err;

  err = pd->dc->ops->init(pd->dc);
  if (err) return err;

  err = pd->rst->ops->init(pd->rst);
  if (err) return err;

  err = pd->backlight->ops->init(pd->backlight);
  if (err) return err;

  err = pd->spi->ops->init(pd->spi);
  if (err) return err;

  err = pd->backlight->ops->write(pd->backlight, PIN_VALUE_0);
  if (err) return err;

  err = hardware_reset(pd);
  if (err) return err;

  err = sleep_out(pd);
  if (err) return err;

  err = normal_mode_on(pd);
  if (err) return err;

  err = display_inversion_off(pd);
  if (err) return err;

  err = memory_data_access_control(pd, 0);
  if (err) return err;

  err = set_pixel_color_format(pd, 16);
  if (err) return err;

  err = display_on(pd);
  if (err) return err;

  uint8_t ids[3] = {0};
  err = read_display_id(pd, ids);
  if (err) return err;
  printf("ST7789V2_ID: 0x%02x%02x%02x;\r\n", ids[0], ids[1], ids[2]);

  uint8_t status[4] = {0};
  err = read_display_status(pd, status);
  if (err) return err;
  printf("ST7789V2_status: 0x%02x%02x%02x%02x;\r\n", status[0], status[1], status[2], status[3]);


  return ESUCCESS;
}

static errno_t on(const Device_ST7789V2 *const pd) {
  return display_on(pd);
}

static errno_t off(const Device_ST7789V2 *const pd) {
  return display_off(pd);
}

static errno_t set_display_memory(Device_ST7789V2 *const pd, uint8_t *memory_ptr, uint32_t memory_size) {
  if (!pd_is_cplt(pd)) return EINVAL;

  pd->display_memory = memory_ptr;
  pd->display_memory_size = memory_size;

  return ESUCCESS;
}

static errno_t set_window(Device_ST7789V2 *const pd, uint16_t start_y, uint16_t start_x, uint16_t end_y, uint16_t end_x) {
  if (!pd_is_cplt(pd)) return EINVAL;
  if (start_x > end_x || start_y > end_y) return EINVAL;

  const uint16_t width = end_x - start_x + 1;
  const uint16_t height = end_y - start_y + 1;

  // 设置的显示范围需要用到的字节数必须小于当前缓冲区的字节数
  if (width * height * pd->one_pixel_byte_num > pd->display_memory_size) return EINVAL;

  errno_t err = ESUCCESS;

  err = col_addr_set(pd, start_x, end_x);
  if (err) return err;
  err = row_addr_set(pd, start_y, end_y);
  if (err) return err;

  pd->window_width = width;
  pd->window_height = height;

  return ESUCCESS;
}

static errno_t set_pixel(const Device_ST7789V2 *const pd, uint16_t y, uint16_t x, color_t color) {
  if (!pd_is_cplt(pd)) return EINVAL;
  if (pd->display_memory == NULL) return EINVAL;
  if (y >= pd->window_height || x >= pd->window_width) return EINVAL;

  const uint32_t byte_idx = (y * pd->window_width + x) * pd->one_pixel_byte_num;
  pd->display_memory[byte_idx] = (uint8_t)(color >> 8);
  pd->display_memory[byte_idx + 1] = (uint8_t)color;

  return ESUCCESS;
}

static errno_t set_ascii_char(Device_ST7789V2 *pds, uint8_t ch, uint16_t start_y, uint16_t start_x, uint16_t color) {
  if (pds == NULL) return EINVAL;
  if (ch >= ASCII_CHAR_COUNT) return EINVAL;

  // 如果起始坐标超出显示窗口范围, 那就什么都不做
  if (start_x >= pds->window_width || start_y >= pds->window_height) return ESUCCESS;
  
  // 获取字体样式
  const ascii_font_t *font = Font_get_ascii(ch);
  if (font == NULL) return EINVAL;

  // 获取显示范围
  uint16_t end_x = start_x + ASCII_CHAR_WIDTH - 1;
  uint16_t end_y = start_y + ASCII_CHAR_HEIGHT - 1;
  if (end_x > pds->window_width - 1) end_x = pds->window_width - 1;
  if (end_y > pds->window_height - 1) end_y = pds->window_height - 1;
  const uint8_t width = (uint8_t)(end_x - start_x + 1);
  const uint8_t height = (uint8_t)(end_y - start_y + 1);

  errno_t err = ESUCCESS;

  for (uint8_t move_y = 0; move_y < height; ++move_y) {
    for (uint8_t move_x = 0; move_x < width; ++move_x) {
      if ((*font)[move_y] & (1 << move_x)) {
        err = pds->ops->set_pixel(pds, start_y + move_y, start_x + move_x, color);
      }
      if (err) return err;
    }
  }

  return ESUCCESS;
}

static errno_t set_ascii_str(Device_ST7789V2 *pds, const uint8_t *const str, uint32_t len, uint16_t start_y, uint16_t start_x, color_t color) {
  if (pds == NULL) return EINVAL;

  // 如果起始纵坐标超出屏幕范围, 那就什么都不做
  if (start_y >= pds->window_height) return ESUCCESS;

  uint16_t cur_x = start_x, cur_y = start_y;
  for (uint32_t i = 0; i < len; ++i) {
    if (str[i] == '\r') {
      cur_x = 0;
      continue;
    }

    if (str[i] == '\n') {
      cur_y += ASCII_CHAR_HEIGHT;
      continue;
    }

    if (cur_x + ASCII_CHAR_WIDTH > pds->window_width) {
      cur_x = 0;
      cur_y += ASCII_CHAR_HEIGHT;
    }

    if (cur_y >= pds->window_height) {
      break;
    }
  
    set_ascii_char(pds, str[i], cur_y, cur_x, color);
    cur_x += ASCII_CHAR_WIDTH;
  }

  return ESUCCESS;
}

static errno_t fill_window(Device_ST7789V2 *const pd, color_t color) {
  if (!pd_is_cplt(pd)) return EINVAL;
  if (pd->display_memory == NULL || pd->window_height == 0 || pd->window_width == 0 || pd->one_pixel_byte_num == 0) return EINVAL;

  errno_t err = ESUCCESS;

  for (uint16_t y = 0; y < pd->window_height; ++y) {
    for (uint16_t x = 0; x < pd->window_width; ++x) {
      err = pd->ops->set_pixel(pd, y, x, color);
      if (err) return err;
    }
  }

  return ESUCCESS;
}

static errno_t refresh_window(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;
  if (pd->display_memory == NULL || pd->window_height == 0 || pd->window_width == 0 || pd->one_pixel_byte_num == 0) return EINVAL;

  errno_t err = memory_write(pd, pd->display_memory, pd->window_height * pd->window_width * pd->one_pixel_byte_num);
  if (err) return err;

  return ESUCCESS;
}

static errno_t clear_screen(Device_ST7789V2 *const pd, color_t color) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  const uint32_t display_memory_size = pd->screen_width * 1 * pd->one_pixel_byte_num;
  uint8_t *const display_memory = (uint8_t *)malloc(display_memory_size);
  if (display_memory == NULL) return ENOMEM;

  memset(display_memory, 0, display_memory_size);

  // 申请变量暂存旧显存数据, 程序结束后恢复设置为旧显存
  uint8_t *const old_display_memory = pd->display_memory;
  const uint32_t old_display_memory_size = pd->display_memory_size;

  err = pd->ops->set_display_memory(pd, display_memory, display_memory_size);
  if (err) goto defer_tag;

  // 先刷新首行
  err = pd->ops->set_window(pd, 0, 0, 0, pd->screen_width - 1);
  if (err) goto defer_tag;

  for (uint16_t x = 0; x < pd->screen_width; ++x) {
    err = pd->ops->set_pixel(pd, 0, x, color);
    if (err) goto defer_tag;
  }
  
  err = pd->ops->refresh_window(pd);
  if (err) goto defer_tag;

  // 再逐次向下移动窗口刷新其余行
  for (uint16_t y = 1; y < pd->screen_height; ++y) {
    err = pd->ops->set_window(pd, y, 0, y, pd->screen_width - 1);
    if (err) {
      goto defer_tag;
    }

    err = pd->ops->refresh_window(pd);
    if (err) {
      goto defer_tag;
    }
  }

  err = ESUCCESS;

  defer_tag:

  free(display_memory);
  pd->ops->set_display_memory(pd, old_display_memory, old_display_memory_size);

  return err;
}

static errno_t hardware_reset(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->rst->ops->write(pd->rst, PIN_VALUE_0);
  if (err) return err;

  // 最少拉低 10 微秒
  err = delay_us(20);
  if (err) return err;

  err = pd->rst->ops->write(pd->rst, PIN_VALUE_1);
  if (err) return err;

  // 在非睡眠模式下复位后需要等待 120 毫秒
  err = delay_ms(120);
  if (err) return err;

  return ESUCCESS;
}

static errno_t software_reset(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_SWRESET);
  if (err) goto reset_cs_tag;

  err = delay_ms(5);
  if (err) return err;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t no_operation(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_NOP);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}


static errno_t read_display_id(const Device_ST7789V2 *const pd, uint8_t rt_ids[3]) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;
  uint8_t rt_data[4] = {0};

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_RDDID);
  if (err) goto reset_cs_tag;

  err = read_data(pd, rt_data, 4);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  rt_ids[0] = rt_data[1];
  rt_ids[1] = rt_data[2];
  rt_ids[2] = rt_data[3];

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t read_display_status(const Device_ST7789V2 *const pd, uint8_t rt_status[4]) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;
  uint8_t rt_data[5] = {0};

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_RDDST);
  if (err) goto reset_cs_tag;

  err = read_data(pd, rt_data, 5);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  rt_status[0] = rt_data[1];
  rt_status[1] = rt_data[2];
  rt_status[2] = rt_data[3];
  rt_status[3] = rt_data[4];

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t sleep_in(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_SLPIN);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t sleep_out(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_SLPOUT);
  if (err) goto reset_cs_tag;

  // 唤醒后 5 毫秒可以发送除了 SLPIN 以外的命令, 120 毫秒后可以发送 SLPIN 命令
  err = delay_ms(5);
  if (err) return err;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t partial_mode_on(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_PTLON);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t normal_mode_on(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_NORON);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t display_inversion_off(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_INVOFF);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t display_inversion_on(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_INVON);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}


static errno_t display_off(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_DISPOFF);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t display_on(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_DISPON);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t col_addr_set(const Device_ST7789V2 *const pd, uint16_t start, uint16_t end) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_CASET);
  if (err) goto reset_cs_tag;

  uint8_t data[4] = { (uint8_t)(start >> 8), (uint8_t)start, (uint8_t)(end >> 8), (uint8_t)end };
  err = write_data(pd, data, 4);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t row_addr_set(const Device_ST7789V2 *const pd, uint16_t start, uint16_t end) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_RASET);
  if (err) goto reset_cs_tag;

  uint8_t data[4] = { (uint8_t)(start >> 8), (uint8_t)start, (uint8_t)(end >> 8), (uint8_t)end };
  err = write_data(pd, data, 4);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t memory_write(const Device_ST7789V2 *const pd, uint8_t *data, uint32_t len) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_RAMWR);
  if (err) goto reset_cs_tag;

  err = write_data(pd, data, len);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t memory_data_access_control(const Device_ST7789V2 *const pd, uint8_t param) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_MADCTL);
  if (err) goto reset_cs_tag;

  err = write_data(pd, &param, 1);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t set_pixel_color_format(const Device_ST7789V2 *const pd, uint8_t bit_num) {
  if (!pd_is_cplt(pd)) return EINVAL;

  uint8_t param = 0;
  switch (bit_num) {
    case 12: param = 0x03; break;
    case 16: param = 0x05; break;
    case 18: param = 0x06; break;
    default: return EINVAL;
  }

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_COLMOD);
  if (err) goto reset_cs_tag;

  err = write_data(pd, &param, 1);
  if (err) goto reset_cs_tag;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t write_register(const Device_ST7789V2 *const pd, const uint8_t cmd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  Pin_value dc_value = PIN_VALUE_1;
  err = pd->dc->ops->read(pd->dc, &dc_value);
  if (err) return err;
  if (dc_value == PIN_VALUE_1) {
    err = pd->dc->ops->write(pd->dc, PIN_VALUE_0);
    if (err) return err;
  }

  err = pd->spi->ops->transmit(pd->spi, &cmd, 1);
  if (err) return err;

  return ESUCCESS;
}

static errno_t write_data(const Device_ST7789V2 *const pd, const uint8_t *data, uint32_t len) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->dc->ops->write(pd->dc, PIN_VALUE_1);
  if (err) return err;

  err = pd->spi->ops->transmit(pd->spi, data, len);
  if (err) goto reset_dc_tag;

  err = pd->dc->ops->write(pd->dc, PIN_VALUE_0);
  if (err) return err;

  return ESUCCESS;

  reset_dc_tag:
  pd->dc->ops->write(pd->dc, PIN_VALUE_0);
  return err;
}

static errno_t read_data(const Device_ST7789V2 *const pd, uint8_t *rt_data, uint32_t len) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->dc->ops->write(pd->dc, PIN_VALUE_1);
  if (err) return err;

  err = pd->spi->ops->receive(pd->spi, rt_data, len);
  if (err) goto reset_dc_tag;

  err = pd->dc->ops->write(pd->dc, PIN_VALUE_0);
  if (err) return err;

  return ESUCCESS;

  reset_dc_tag:
  pd->dc->ops->write(pd->dc, PIN_VALUE_0);
  return err;
}

static inline uint8_t pd_is_cplt(const Device_ST7789V2 *const pd) {
  return (
    pd != NULL
    && pd->ops != NULL
    && pd->cs != NULL
    && pd->dc != NULL
    && pd->rst != NULL
    && pd->backlight != NULL
    && pd->spi != NULL
  );
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_ST7789V2 *)pd)->name == *((Device_ST7789V2_name *)name);
}
