#include "wifi_bluetooth.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define WAIT_ACT_BUFFER_SIZE 100

// 协议常量（文件内可见）
static const char ACK_OK[]  = "\r\nOK\r\n";
static const char ACK_ERR[] = "\r\nERROR\r\n";
// 不同模块文档存在两种写法："Unknown cmd:" 与 "Unknowncmd:"，这里同时支持
static const char ACK_UNK_A[] = "Unknown cmd:";
static const char ACK_UNK_B[] = "Unknowncmd:";
enum {
  ACK_OK_LEN = 6,
  ACK_ERR_LEN = 9,
  ACK_UNK_A_LEN = 12,
  ACK_UNK_B_LEN = 11,
  // 滑窗保留的尾部长度，至少覆盖最长固定前缀，避免跨分片匹配丢失
  ACK_MAX_LEN = 12
};

// 对象方法
static errno_t init(Device_wifi_bluetooth *const pd);

// 内部方法
// 发送指令
static errno_t send_cmd(
  Device_wifi_bluetooth *const pd
  , uint8_t *cmd
  , uint32_t cmd_len
  , uint32_t timeout_ms
  , uint8_t *p_rt_data
  , uint32_t *p_rt_data_len
  , uint32_t rt_data_size
);
// 等待指令响应
static errno_t wait_cmd_ack(
  Device_wifi_bluetooth *const pd
  , uint32_t timeout_ms
  , uint8_t *p_rt_data
  , uint32_t *p_rt_data_len
  , uint32_t rt_data_size
);
// 下面 3 个是 wait_cmd_ack 的辅助函数
static void append_output(
  const uint8_t *src
  , uint32_t emit
  , uint8_t *out
  , uint32_t *out_len_ptr
  , uint32_t out_cap
  , bool *truncated_ptr
);
static void slide_window(
  uint8_t *buf
  , uint32_t *idx_ptr
  , uint32_t keep
  , uint8_t *out
  , uint32_t *out_len_ptr
  , uint32_t out_cap
  , bool *truncated_ptr
);
static int try_match_ack(
  const uint8_t *buf
  , uint32_t idx
  , uint8_t *out
  , uint32_t *out_len_ptr
  , uint32_t out_cap
  , bool *truncated_ptr
  , errno_t *ret_errno
);

// 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_wifi_bluetooth_ops device_ops = {
  .init = init,
};

static List *list = NULL;

errno_t Device_wifi_bluetooth_module_init(void) {
  if (list == NULL) {
    errno_t err = list_create(&list);
    if (err) return err;
  }

  return ESUCCESS;
}

errno_t Device_wifi_bluetooth_register(Device_wifi_bluetooth *const pd) {
  if (pd == NULL || list == NULL) return EINVAL;
  pd->ops = &device_ops;
  list->ops->head_insert(list, pd);
  return ESUCCESS;
}

errno_t Device_wifi_bluetooth_find(Device_wifi_bluetooth **pd_ptr, const Device_wifi_bluetooth_name name) {
  if (list == NULL) return EINVAL;

  errno_t err = list->ops->find(list, pd_ptr, &name, match_device_by_name);
  if (err) return err;

  return ESUCCESS;
}

static inline uint8_t match_device_by_name(const void *const name, const void *const pd) {
  return ((Device_wifi_bluetooth *)pd)->name == *((Device_wifi_bluetooth_name *)name);
}

static errno_t init(Device_wifi_bluetooth *const pd) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->usart->ops->init(pd->usart);
  if (err) return err;

  err = pd->timer->ops->init(pd->timer);
  if (err) return err;
  err = pd->timer->ops->set_period(pd->timer, 1000);
  if (err) return err;
  err = pd->timer->ops->start(pd->timer, DEVICE_TIMER_START_MODE_IT);
  if (err) return err;

  return ESUCCESS;
}

static errno_t join_wifi_ap(Device_wifi_bluetooth *const pd, const uint8_t *const ssid, uint8_t ssid_len,  const uint8_t *const pwd, uint8_t pwd_len) {
  if (pd == NULL) return EINVAL;

  const uint8_t cmd_size = 100;
  uint8_t cmd[100] = {0};

  int32_t cmd_len = snprintf((char *)cmd, 100, "AT+WJAP=%s,%s\r\n", ssid, pwd);
  if (cmd_len >= cmd_size) {
    return EOVERFLOW;
  }

  errno_t err = ESUCCESS;

  err = send_cmd(pd, cmd, cmd_len, 1000, NULL, NULL, 0);
  if (err) return err;


  return ESUCCESS;
}

static errno_t create_socket_connection(Device_wifi_bluetooth *const pd, const uint8_t) {
  if (pd == NULL) return EINVAL;
}

/**
 * @brief 发送指令
 * @param pd wifi 设备
 * @param cmd 指令
 * @param cmd_len 指令长度
 * @param timeout_ms 最大等待时长, 单位毫秒
 * @param p_rt_data 接收返回数据的指针, 不需要接收返回数据传入 NULL
 * @param p_rt_data_len 接收返回数据长度的指针, 当 p_rt_data == NULL 时会忽略
 * @param rt_data_size 接收返回数据的指针对应的内存空间大小, 当 p_rt_data == NULL 时会忽略该参数
 * @return errno_t 错误码, 返回 ESUCCESS 时表示成功, 返回其他表示失败
 */
static errno_t send_cmd(
  Device_wifi_bluetooth *const pd
  , uint8_t *cmd
  , uint32_t cmd_len
  , uint32_t timeout_ms
  , uint8_t *p_rt_data
  , uint32_t *p_rt_data_len
  , uint32_t rt_data_size
) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->usart->ops->transmit(pd->usart, cmd, cmd_len);
  if (err) return err;

  err = wait_cmd_ack(pd, timeout_ms, p_rt_data, p_rt_data_len, rt_data_size);
  if (err) return err;

  return ESUCCESS;
}

/**
 * @brief 等待指令响应
 * @param pd wifi 设备
 * @param timeout_ms 最大等待时长, 单位毫秒
 * @param p_rt_data 接收返回数据的指针, 不需要接收返回数据传入 NULL
 * @param p_rt_data_len 接收返回数据长度的指针, 当 p_rt_data == NULL 时会忽略
 * @param rt_data_size 接收返回数据的指针对应的内存空间大小, 当 p_rt_data == NULL 时会忽略该参数
 * @return errno_t 错误码, 返回 ESUCCESS 时表示成功, 返回其他表示失败
 */
static errno_t wait_cmd_ack(
  Device_wifi_bluetooth *const pd
  , uint32_t timeout_ms
  , uint8_t *p_rt_data
  , uint32_t *p_rt_data_len
  , uint32_t rt_data_size
) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;
  uint32_t t0 = 0, now = 0;
  err = pd->timer->ops->get_count(pd->timer, &t0);
  if (err) return err;

  uint8_t buf[WAIT_ACT_BUFFER_SIZE] = {0};
  uint32_t idx = 0;           // buf 已用长度
  uint32_t out_len = 0;       // 已写到 p_rt_data 的长度
  bool truncated = false;     // 输出是否截断

  for (;;) {
    // 超时
    err = pd->timer->ops->get_count(pd->timer, &now);
    if (err) return err;
    if ((uint32_t)(now - t0) >= timeout_ms) {
      if (p_rt_data_len) *p_rt_data_len = out_len;
      return ETIMEDOUT;
    }

    // 缓冲区将满：滑窗，仅保留 ACK 匹配所需尾部
    if (idx >= WAIT_ACT_BUFFER_SIZE - 1) {
      uint32_t keep = (ACK_MAX_LEN > 1) ? (ACK_MAX_LEN - 1) : 1;
      slide_window(buf, &idx, keep, p_rt_data, &out_len, rt_data_size, &truncated);
    }

    // 读取串口数据，注意剩余空间
    uint32_t rd = 0;
    err = pd->usart->ops->receive(pd->usart, buf + idx, &rd, (uint32_t)(WAIT_ACT_BUFFER_SIZE - idx));
    if (err) return err;
    if (rd == 0) continue;
    idx += rd;

    errno_t matched_errno = ESUCCESS;
    int matched = try_match_ack(
      buf, idx, p_rt_data, &out_len, rt_data_size
      , &truncated, &matched_errno
    );
    if (matched) {
      if (p_rt_data_len) *p_rt_data_len = out_len;
      return matched_errno;
    }
    // 未找到，继续下一轮（跨边界匹配靠尾部 keep 保证）
  }
}

/**
 * @brief 滑动接收缓冲区，仅保留尾部 keep 字节
 *
 * 将循环缓冲区中除尾部 keep 字节以外的“前部数据”作为历史输出（通过 append_output 写入 out），
 * 然后把保留的尾部字节搬移到缓冲区起始处，以便继续接收后续数据并支持跨分片匹配。
 *
 * 设计目的：当接收缓冲区将满，但仍需继续接收数据并匹配应答前缀（如 "\r\nOK\r\n"），
 * 通过保留足以完成前缀匹配的尾部片段，避免因边界切分而丢失匹配机会，同时把已无需参与匹配的历史字节发射到外部输出。
 *
 * @param buf         接收缓冲区
 * @param idx_ptr     [入/出] 当前缓冲区已用长度，执行后会被更新为 keep
 * @param keep        需要在缓冲区内保留的尾部字节数（若大于当前已用长度会被截断到 idx）
 * @param out         历史数据的输出缓冲区（可为 NULL，表示丢弃历史数据）
 * @param out_len_ptr [入/出] 输出缓冲区当前已用长度指针（out 非空时必须有效）
 * @param out_cap     输出缓冲区总容量（字节）
 * @param truncated_ptr 若历史数据因容量不足被截断，则置为 true（可为 NULL）
 */
static void slide_window(
  uint8_t *buf
  , uint32_t *idx_ptr
  , uint32_t keep
  , uint8_t *out
  , uint32_t *out_len_ptr
  , uint32_t out_cap
  , bool *truncated_ptr
) {
  uint32_t idx = *idx_ptr;
  if (keep > idx) keep = idx;
  uint32_t emit = idx - keep;
  append_output(buf, emit, out, out_len_ptr, out_cap, truncated_ptr);
  memmove(buf, buf + (idx - keep), keep);
  *idx_ptr = keep;
}

/**
 * @brief 在接收缓冲区中尝试匹配 ACK/错误/未知命令等固定响应
 *
 * 支持匹配以下几类：
 * - OK:    "\r\nOK\r\n"，匹配到后会将其之前的历史字节发射到 out，并以 ESUCCESS（或 EOVERFLOW）返回
 * - ERROR: "\r\nERROR\r\n"，匹配到后以 EIO 返回
 * - Unknown cmd: "Unknown cmd:" / "Unknowncmd:"，会采集其后直到 CRLF 的字符串到 out，并以 EINVAL（或 EOVERFLOW）返回
 *
 * 注意：当输出缓冲区容量不足导致历史或未知信息被截断时，会通过 truncated_ptr 进行标记，
 * 对应的返回 errno 也会变为 EOVERFLOW，以提示上层“响应有效但输出被截断”。
 *
 * @param buf            接收缓冲区
 * @param idx            当前缓冲区已用长度
 * @param out            输出缓冲区（可为 NULL，表示仅做匹配判定不保留内容）
 * @param out_len_ptr    [入/出] 输出缓冲区已用长度
 * @param out_cap        输出缓冲区容量
 * @param truncated_ptr  截断标记指针（可为 NULL）
 * @param ret_errno      [出] 根据匹配类型返回对应错误码/成功码
 * @return int           1 表示已匹配到终止条件（OK/ERROR/Unknown*），0 表示未匹配，需继续接收
 */
static int try_match_ack(
  const uint8_t *buf
  , uint32_t idx
  , uint8_t *out
  , uint32_t *out_len_ptr
  , uint32_t out_cap
  , bool *truncated_ptr
  , errno_t *ret_errno
) {
  for (uint32_t i = 0; i < idx; ++i) {
    // OK
    if (i + ACK_OK_LEN <= idx && memcmp(buf + i, ACK_OK, ACK_OK_LEN) == 0) {
      append_output(buf, i, out, out_len_ptr, out_cap, truncated_ptr);
      *ret_errno = (truncated_ptr && *truncated_ptr) ? EOVERFLOW : ESUCCESS;
      return 1;
    }

    // ERROR
    if (i + ACK_ERR_LEN <= idx && memcmp(buf + i, ACK_ERR, ACK_ERR_LEN) == 0) {
      *ret_errno = EIO;
      return 1;
    }

    // Unknown cmd / Unknowncmd：采集前缀后的到 CRLF 的内容
    if (i + ACK_UNK_A_LEN <= idx && memcmp(buf + i, ACK_UNK_A, ACK_UNK_A_LEN) == 0) {
      uint32_t start = i + ACK_UNK_A_LEN;
      for (uint32_t j = start; j + 1 < idx; ++j) {
        if (buf[j] == '\r' && buf[j + 1] == '\n') {
          append_output(buf + start, j - start, out, out_len_ptr, out_cap, truncated_ptr);
          *ret_errno = (truncated_ptr && *truncated_ptr) ? EOVERFLOW : EINVAL;
          return 1;
        }
      }
    }

    if (i + ACK_UNK_B_LEN <= idx && memcmp(buf + i, ACK_UNK_B, ACK_UNK_B_LEN) == 0) {
      uint32_t start = i + ACK_UNK_B_LEN;
      for (uint32_t j = start; j + 1 < idx; ++j) {
        if (buf[j] == '\r' && buf[j + 1] == '\n') {
          append_output(buf + start, j - start, out, out_len_ptr, out_cap, truncated_ptr);
          *ret_errno = (truncated_ptr && *truncated_ptr) ? EOVERFLOW : EINVAL;
          return 1;
        }
      }
    }

  }
  return 0;
}

/**
 * @brief 将 emit 字节从 src 追加写入外部输出缓冲区（带容量保护）
 *
 * 行为约定：
 * - 当 out/out_len_ptr 无效或 out_cap 已满时，不写入数据并将 truncated 置位（若提供）。
 * - 实际拷贝字节数为 min(emit, out_cap - *out_len_ptr)。若不足以容纳全部 emit，则视为被截断。
 * - emit 为 0 时直接返回。
 *
 * @param src            源数据指针
 * @param emit           需要发射/追加的字节数
 * @param out            目标输出缓冲区（可为 NULL 表示丢弃）
 * @param out_len_ptr    [入/出] 输出缓冲区已用长度指针
 * @param out_cap        输出缓冲区容量
 * @param truncated_ptr  若发生丢弃/截断则置为 true（可为 NULL）
 */
static void append_output(
  const uint8_t *src
  , uint32_t emit
  , uint8_t *out
  , uint32_t *out_len_ptr
  , uint32_t out_cap
  , bool *truncated_ptr
) {
  if (emit == 0) return;

  if (!(out && out_len_ptr)) {
    if (truncated_ptr) *truncated_ptr = true;
    return;
  }

  uint32_t out_len = *out_len_ptr;
  uint32_t cancpy = (out_cap > out_len) ? (out_cap - out_len) : 0;

  if (!cancpy) {
    if (truncated_ptr) *truncated_ptr = true;
    return;
  }

  if (cancpy > emit) cancpy = emit;
  memcpy(out + out_len, src, cancpy);
  *out_len_ptr = out_len + cancpy;
  if (emit > cancpy && truncated_ptr) *truncated_ptr = true;
}
