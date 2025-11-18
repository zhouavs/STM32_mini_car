#include "st7789v2.h"
#include "cmd.h"
#include "common/delay/delay.h"
#include "common/list/list.h"
#include <stdlib.h>
#include <stdio.h>

// 对象方法
static errno_t init(const Device_ST7789V2 *const pd);
static errno_t on(const Device_ST7789V2 *const pd);
static errno_t off(const Device_ST7789V2 *const pd);
static errno_t set_display_window(Device_ST7789V2 *const pd, uint16_t start_y, uint16_t start_x, uint16_t end_y, uint16_t end_x);
static errno_t set_pixel(const Device_ST7789V2 *const pd, uint16_t y, uint16_t x, uint16_t color);
static errno_t refresh_window(const Device_ST7789V2 *const pd);

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
  .set_display_window = set_display_window,
  .set_pixel = set_pixel,
  .refresh_window = refresh_window,
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


errno_t init(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->init(pd->cs);
  if (err) return err;

  err = pd->dc->ops->init(pd->dc);
  if (err) return err;

  err = pd->rst->ops->init(pd->rst);
  if (err) return err;

  err = pd->spi->ops->init(pd->spi);
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


  return ESUCCESS;
}

errno_t on(const Device_ST7789V2 *const pd) {
  return display_on(pd);
}

errno_t off(const Device_ST7789V2 *const pd) {
  return display_off(pd);
}

errno_t set_display_window(Device_ST7789V2 *const pd, uint16_t start_y, uint16_t start_x, uint16_t end_y, uint16_t end_x) {
  if (!pd_is_cplt(pd)) return EINVAL;
  if (start_x > end_x || start_y > end_y) return EINVAL;

  const uint16_t width = end_x - start_x + 1;
  const uint16_t height = end_y - start_y + 1;

  // 设置的显示范围需要用到的字节数必须小于当前缓冲区的字节数
  if (width * height * pd->one_pixel_byte_num > pd->pixel_bytes_size) return EINVAL;

  errno_t err = ESUCCESS;

  err = col_addr_set(pd, start_x, end_x);
  if (err) return err;
  err = row_addr_set(pd, start_y, end_y);
  if (err) return err;

  pd->width = width;
  pd->height = height;

  return ESUCCESS;
}

errno_t set_pixel(const Device_ST7789V2 *const pd, uint16_t y, uint16_t x, uint16_t color) {
  if (!pd_is_cplt(pd)) return EINVAL;
  if (pd->pixel_bytes == NULL) return EINVAL;
  if (y >= pd->height || x >= pd->width) return EINVAL;

  const uint32_t byte_idx = (y * pd->width + x) * pd->one_pixel_byte_num;
  pd->pixel_bytes[byte_idx] = (uint8_t)(color >> 8);
  pd->pixel_bytes[byte_idx + 1] = (uint8_t)color;

  return ESUCCESS;
}

errno_t refresh_window(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;
  if (pd->pixel_bytes == NULL || pd->height == 0 || pd->width == 0 || pd->one_pixel_byte_num == 0) return EINVAL;

  errno_t err = memory_write(pd, pd->pixel_bytes, pd->height * pd->width * pd->one_pixel_byte_num);
  if (err) return err;

  return ESUCCESS;
}


static errno_t hardware_reset(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->rst->ops->write(pd->rst, PIN_VALUE_0);
  if (err) return err;

  // 最少拉低 10 微秒
  delay_ms(1);

  err = pd->rst->ops->write(pd->rst, PIN_VALUE_1);
  if (err) return err;

  // 在非睡眠模式下复位后需要等待 120 毫秒
  delay_ms(120);

  return ESUCCESS;
}

static errno_t software_reset(const Device_ST7789V2 *const pd) {
  if (!pd_is_cplt(pd)) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = write_register(pd, ST7789V2_CMD_SWRESET);
  if (err) goto reset_cs_tag;

  delay_ms(5);

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
  delay_ms(5);

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

  uint8_t data[4] = { (uint8_t)start >> 8, (uint8_t)start, (uint8_t)end >> 8, (uint8_t)end };
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

  uint8_t data[4] = { (uint8_t)start >> 8, (uint8_t)start, (uint8_t)end >> 8, (uint8_t)end };
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
    && pd->spi != NULL
  );
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_ST7789V2 *)pd)->name == *((Device_ST7789V2_name *)name);
}
