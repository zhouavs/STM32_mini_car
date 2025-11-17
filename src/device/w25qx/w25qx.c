#include "w25qx.h"
#include "cmd.h"
#include "device/gpio/gpio.h"
#include "config/index.h"
#include <stdint.h>
#include <stdlib.h>
#include "common/list/list.h"

// 对象方法
static errno_t init(const Device_W25QX *const pd);
static errno_t erase(const Device_W25QX *const pd, uint32_t addr, uint16_t sector_count);
static errno_t read(const Device_W25QX *const pd, uint32_t addr, uint8_t *data, uint32_t len);
static errno_t write(const Device_W25QX *const pd, uint32_t addr, uint8_t *data, uint32_t len);

// 内部方法 - 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);
// 内部方法 - W25QX 操作
static errno_t read_id(const Device_W25QX *const pd, uint32_t *id_ptr);
static errno_t power_down(const Device_W25QX *const pd) __attribute__((unused));
static errno_t release_power_down(const Device_W25QX *const pd) __attribute__((unused));
static errno_t write_enable(const Device_W25QX *const pd);
static errno_t write_disable(const Device_W25QX *const pd) __attribute__((unused));
static errno_t wait_write_complete(const Device_W25QX *const pd);
static errno_t sector_erase(const Device_W25QX *const pd, uint32_t addr);
static errno_t block_erase_32k(const Device_W25QX *const pd, uint32_t addr) __attribute__((unused));
static errno_t block_erase_64k(const Device_W25QX *const pd, uint32_t addr) __attribute__((unused));
static errno_t chip_erase(const Device_W25QX *const pd) __attribute__((unused));
static errno_t page_write(const Device_W25QX *const pd, uint32_t addr, uint8_t *data, uint16_t len);
// 内部方法 - 地址由整型转为字节数组
static errno_t addr_to_bytes(uint32_t addr, uint8_t *bytes);

// 全局变量
static List *list = NULL;
static const Device_W25QX_ops device_ops = {
  .init = init,
  .erase = erase,
  .read = read,
  .write = write,
};

errno_t Device_W25QX_module_init() {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_W25QX_register(Device_W25QX *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_W25QX_find(const Device_W25QX **pd_ptr, const Device_W25QX_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static errno_t init(const Device_W25QX *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->cs->ops->init(pd->cs);
  if (err) return err;
  err = pd->spi->ops->init(pd->spi);
  if (err) return err;

  uint32_t id = 0;
  err = read_id(pd, &id);
  if (err) return err;

  if (id != W25QX_ID) return E_CUSTOM_W25QX_DEVICE_ID_ERROR;

  return ESUCCESS;
}

static errno_t erase(const Device_W25QX *const pd, uint32_t addr, uint16_t sector_count) {
  if (pd == NULL) return EINVAL;
  // 地址必须是扇区的起始地址
  if (addr % W25QX_SECTOR_SIZE != 0) return E_CUSTOM_W25QX_ADDR_ERROR;
  // 从 addr 开始的剩余扇区数量必须大于传入的扇区数量, 由于乘法性能高于除法, 此处比较字节数
  if (W25QX_SIZE - addr < sector_count * W25QX_SECTOR_SIZE) return E_CUSTOM_W25QX_OVERSTEP;

  while (sector_count--) {
    errno_t err = sector_erase(pd, addr);
    if (err) return err;
    addr += W25QX_SECTOR_SIZE;
  }

  return ESUCCESS;
}

static errno_t read(const Device_W25QX *const pd, uint32_t addr, uint8_t *data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  // 从 addr 开始的剩余字节数必须大于 len
  if (W25QX_SIZE - addr < len) return E_CUSTOM_W25QX_OVERSTEP;

  errno_t err = ESUCCESS;

  uint8_t cmd_addr[4] = {W25QX_CMD_READ_DATA};
  err = addr_to_bytes(addr, cmd_addr + 1);
  if (err) return err;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, cmd_addr, 4);
  if (err) goto reset_cs_tag;
  err = pd->spi->ops->receive(pd->spi, data, len);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t write(const Device_W25QX *const pd, uint32_t addr, uint8_t *data, uint32_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  // 从 addr 开始的剩余字节数必须大于 len
  if (W25QX_SIZE - addr < len) return E_CUSTOM_W25QX_OVERSTEP;

  errno_t err = ESUCCESS;

  // 擦除将会写入的扇区
  // 获取擦除起始地址对应的扇区 扇区数量 起始扇区地址
  const uint16_t start_sector = addr / W25QX_SECTOR_SIZE;
  uint16_t sector_count = (addr + len) / W25QX_SECTOR_SIZE - start_sector + 1;
  uint32_t sector_addr = start_sector * W25QX_SECTOR_SIZE;
  while (sector_count--) {
    err = sector_erase(pd, sector_addr);
    if (err) return err;
    sector_addr += W25QX_SECTOR_SIZE;
  }

  // 分页写入
  // 获取第一页可以写入的字节数
  const uint16_t first_page_remain = W25QX_PAGE_SIZE - (addr % W25QX_PAGE_SIZE);
  uint16_t cur_len = 0;
  if (len > first_page_remain) {
    cur_len = first_page_remain;
    len -= cur_len;
  } else {
    cur_len = len;
    len = 0;
  }
  err = page_write(pd, addr, data, cur_len);
  if (err) return err;

  while (len) {
    addr += cur_len;
    data += cur_len;
    if (len > W25QX_PAGE_SIZE) {
      cur_len = W25QX_PAGE_SIZE;
      len -= W25QX_PAGE_SIZE;
    } else {
      cur_len = len;
      len = 0;
    }
    err = page_write(pd, addr, data, cur_len);
    if (err) return err;
  }

  return ESUCCESS;
}


static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_W25QX *)pd)->name == *((Device_W25QX_name *)name);
}

static errno_t read_id(const Device_W25QX *const pd, uint32_t *id_ptr) {
  if (pd == NULL || id_ptr == NULL) return EINVAL;

  uint8_t cmd = W25QX_CMD_JEDEC_ID;
  uint8_t data[3] = {0};

  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, &cmd, 1);
  if (err) goto reset_cs_tag;
  err = pd->spi->ops->receive(pd->spi, data, 3);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  // 先返回的高位数据, 后返回的低位数据
  *id_ptr = (data[0] << 16) | (data[1] << 8) | (data[2]);

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t power_down(const Device_W25QX *const pd) {
  if (pd == NULL) return EINVAL;

  uint8_t cmd = W25QX_CMD_POWER_DOWN;
  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, &cmd, 1);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t release_power_down(const Device_W25QX *const pd) {
  if (pd == NULL) return EINVAL;

  uint8_t cmd = W25QX_CMD_RELEASE_POWER_DOWN;
  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, &cmd, 1);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t write_enable(const Device_W25QX *const pd) {
  if (pd == NULL) return EINVAL;

  uint8_t cmd = W25QX_CMD_WRITE_ENABLE;
  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, &cmd, 1);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t write_disable(const Device_W25QX *const pd) {
  if (pd == NULL) return EINVAL;

  uint8_t cmd = W25QX_CMD_WRITE_DISABLE;
  errno_t err = ESUCCESS;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, &cmd, 1);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t wait_write_complete(const Device_W25QX *const pd) {
  if (pd == NULL) return EINVAL;

  uint8_t cmd = W25QX_CMD_READ_STATUS_REGISTER_1;
  errno_t err = ESUCCESS;
  uint8_t status = 0;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;

  err = pd->spi->ops->transmit(pd->spi, &cmd, 1);
  if (err) goto reset_cs_tag;

  do {
    err = pd->spi->ops->receive(pd->spi, &status, 1);
    if (err) goto reset_cs_tag;
  } while ((status & 0x01) == 1);

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;

  return ESUCCESS;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t sector_erase(const Device_W25QX *const pd, uint32_t addr) {
  if (pd == NULL) return EINVAL;
  // 必须为扇区的起始地址
  if (addr % W25QX_SECTOR_SIZE != 0) return E_CUSTOM_W25QX_ADDR_ERROR;

  errno_t err = ESUCCESS;

  uint8_t data[4] = { W25QX_CMD_SECTOR_ERASE };
  err = addr_to_bytes(addr, data + 1);
  if (err) return err;

  err = write_enable(pd);
  if (err) return err;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, data, 4);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;
  err = wait_write_complete(pd);
  if (err) return err;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t block_erase_32k(const Device_W25QX *const pd, uint32_t addr) {
  if (pd == NULL) return EINVAL;
  // 必须为 32k 空间的起始地址
  if (addr % 0x8000 != 0) return E_CUSTOM_W25QX_ADDR_ERROR;

  errno_t err = ESUCCESS;

  uint8_t data[4] = { W25QX_CMD_BLOCK_ERASE_32K };
  err = addr_to_bytes(addr, data + 1);
  if (err) return err;

  err = write_enable(pd);
  if (err) return err;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, data, 4);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;
  err = wait_write_complete(pd);
  if (err) return err;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t block_erase_64k(const Device_W25QX *const pd, uint32_t addr) {
  if (pd == NULL) return EINVAL;
  // 必须为 64k 空间的起始地址
  if (addr % 0x10000 != 0) return E_CUSTOM_W25QX_ADDR_ERROR;

  errno_t err = ESUCCESS;

  uint8_t data[4] = { W25QX_CMD_BLOCK_ERASE_64K };
  err = addr_to_bytes(addr, data + 1);
  if (err) return err;

  err = write_enable(pd);
  if (err) return err;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, data, 4);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;
  err = wait_write_complete(pd);
  if (err) return err;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t chip_erase(const Device_W25QX *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;
  uint8_t cmd = W25QX_CMD_CHIP_ERASE;

  err = write_enable(pd);
  if (err) return err;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, &cmd, 1);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;
  err = wait_write_complete(pd);
  if (err) return err;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t page_write(const Device_W25QX *const pd, uint32_t addr, uint8_t *data, uint16_t len) {
  if (pd == NULL || data == NULL || len == 0) return EINVAL;
  // 检查片写是否越界
  if (addr % 0x100 + len > 0x100) return E_CUSTOM_W25QX_OVERSTEP;

  errno_t err = ESUCCESS;

  uint8_t cmd_addr[4] = {W25QX_CMD_PAGE_PROGRAM};
  err = addr_to_bytes(addr, cmd_addr + 1);
  if (err) return err;

  err = write_enable(pd);
  if (err) return err;

  err = pd->cs->ops->write(pd->cs, PIN_VALUE_0);
  if (err) return err;
  err = pd->spi->ops->transmit(pd->spi, cmd_addr, 4);
  if (err) goto reset_cs_tag;
  err = pd->spi->ops->transmit(pd->spi, data, len);
  if (err) goto reset_cs_tag;
  err = pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  if (err) return err;
  err = wait_write_complete(pd);
  if (err) return err;

  reset_cs_tag:
  pd->cs->ops->write(pd->cs, PIN_VALUE_1);
  return err;
}

static errno_t addr_to_bytes(uint32_t addr, uint8_t *bytes) {
  if (bytes == NULL) return EINVAL;
  bytes[0] = (addr & 0xFF0000) >> 16;
  bytes[1] = (addr & 0x00FF00) >> 8;
  bytes[2] = (addr & 0x0000FF);
  return ESUCCESS;
}
