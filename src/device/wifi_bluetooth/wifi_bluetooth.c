#include "wifi_bluetooth.h"
#include "common/list/list.h"
#include "common/delay/delay.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

#define WAIT_ACT_BUFFER_SIZE 0x100u

/**
 * @brief wifi-蓝牙 设备使用的自定义字符串
 */
typedef struct {
  uint8_t *buf;
  uint32_t len;
  uint32_t size;
} wb_string;

static errno_t wb_string_concat(wb_string *const aim, const wb_string *const from);

typedef enum {
  MATCHED_MARK_SUCCESS,
  MATCHED_MARK_FAIL,
} Matched_mark;

// 匹配等待响应的函数
typedef errno_t (match_fn_t)(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);

// 对象方法
static errno_t init(Device_wifi_bluetooth *const pd);

// 内部方法
// 发送指令
static errno_t send_cmd(Device_wifi_bluetooth *const pd, wb_string *cmd, wb_string *rt_data, uint32_t timeout_ms);
// 等待直到收到特定的响应信息
static errno_t wait_ack(
  Device_wifi_bluetooth *const pd // wifi 设备
  , wb_string *rt_data
  , uint32_t timeout_ms // 超时时间
  , match_fn_t *const match_fns[]
  , uint8_t match_fn_count
);
// 匹配指令执行成功响应信息
errno_t match_cmd_success(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 匹配指令执行错误响应信息
errno_t match_cmd_error(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 匹配响应未知指令信息
errno_t match_cmd_unknown(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 匹配获取到 IP 事件
errno_t match_event_got_ip(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 从后向前匹配字符串
void rstrstr(const wb_string *haystack, const wb_string *needle, bool *rt_matched_ptr, uint32_t *rt_idx_ptr);

// 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_wifi_bluetooth_ops device_ops = {
  .init = init,
};

static List *list = NULL;
// socket 连接节点列表
static List *net_node_list[DEVICE_WIFI_BLUETOOTH_COUNT] = {
  [DEVICE_WIFI_BLUETOOTH_1] = NULL,
};

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

  if (net_node_list[pd->name] == NULL) {
    err = list_create(&net_node_list[pd->name]);
    if (err) return err;
  }

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

static errno_t join_wifi_ap(Device_wifi_bluetooth *const pd, const uint8_t *const ssid, const uint8_t *const pwd) {
  if (pd == NULL) return EINVAL;

  uint8_t cmd_buf[100] = {0};
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 100 };

  cmd.len = snprintf((char *)cmd_buf, cmd.size, "AT+WJAP=%s,%s\r\n", ssid, pwd);
  if (cmd.len >= cmd.size) {
    return EOVERFLOW;
  }

  errno_t err = ESUCCESS;

  err = send_cmd(pd, &cmd, NULL, 1000);
  if (err) return err;

  match_fn_t *const match_fns[] = { match_event_got_ip };
  err = wait_ack(pd, NULL, 5000, match_fns, sizeof(match_fns) / sizeof(match_fn_t *));
  if (err) return err;

  return ESUCCESS;
}

static errno_t create_socket_connection(
  Device_wifi_bluetooth *const pd
  , Device_wifi_bluetooth_socket_type type
  , uint8_t *remote_host
  , uint16_t port
) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  uint8_t cmd_buf[50] = {0};
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 50 };

  Net_node *p_net_node = (Net_node *)malloc(sizeof(Net_node));
  p_net_node->type = type;
  p_net_node->port = port;

  switch (type) {
    case UDP_CLIENT:
    case TCP_CLIENT:
    case SSL_CLIENT: {
      if (remote_host == NULL) {
        err = EINVAL;
        goto err_flag;
      }

      cmd.len = snprintf((char *)cmd_buf, cmd.size, "AT+SOCKET=%d,%s,%d\r\n", type, remote_host, port);

      // 保存 host
      uint16_t host_len = strlen((char *)remote_host);
      p_net_node->remote_host = malloc(host_len + 1);
      memcpy(p_net_node->remote_host, remote_host, host_len);
      p_net_node->remote_host[host_len] = '\0';

      break;
    }
    case UDP_SERVER:
    case TCP_SERVER:
    case SSL_SERVER: {
      cmd.len = snprintf((char *)cmd_buf, cmd.size, "AT+SOCKET=%d,%d\r\n", type, port);
      break;
    }
    default: {
      err = EINVAL;
      goto err_flag;
    }
  }

  if (cmd.len >= cmd.size) {
    err = EOVERFLOW;
    goto err_flag;
  }

  uint8_t data_buf[50] = {0};
  wb_string data = { .buf = data_buf, .len = 0, .size = 50 };

  err = send_cmd(pd, &cmd, &data, 5000);
  if (err) goto err_flag;;

  const char *pre_str = "connect success ConID=";
  const uint8_t pre_str_len = strlen(pre_str);

  if (data.len + 1 > data.size) {
    err = EOVERFLOW;
    goto err_flag;
  }
  data.buf[data.len] = '\0';

  char *con_id_start = strstr((char *)data.buf, pre_str);
  if (con_id_start == NULL) {
    printf("create_socket_connection_fail_no_conId -> %s\r\n", data.buf);
    err = EIO;
    goto err_flag;
  }
  con_id_start += pre_str_len;

  char *con_id_end = NULL;
  errno = 0;
  const long con_id = strtol(con_id_start, &con_id_end, 10);
  // 没有可转换为数字的子字符串
  if (con_id_end == con_id_start) {
    printf("create_socket_connection_fail_no_num_str -> %s\r\n", data.buf);
    err = EIO;
    goto err_flag;
  }
  // 转换溢出
  if (errno == ERANGE) {
    printf("create_socket_connection_fail_overflow -> %s\r\n", data.buf);
    err = EIO;
    goto err_flag;
  }
  if (con_id < 0) {
    printf("create_socket_connection_fail_conId_err ->%s\r\n", data.buf);
    err = EIO;
    goto err_flag;
  }

  err = net_node_list[pd->name]->ops->head_insert(net_node_list[pd->name], p_net_node);
  if (err) goto err_flag;

  return ESUCCESS;

  err_flag:
  if (p_net_node) {
    if (p_net_node->remote_host) {
      free(p_net_node->remote_host);
    }
    free(p_net_node);
  }
  return err;
}

static errno_t delete_socket_connection(Device_wifi_bluetooth *const pd, uint32_t connect_id) {
  if (pd == NULL) return EINVAL;

  uint8_t cmd_buf[25] = {0};
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 25 };

  cmd.len = snprintf((char *)cmd.buf, cmd.size, "AT+SOCKETDEL=%d", connect_id);

  if (cmd.len >= cmd.size) {
    return EOVERFLOW;
  }

  errno_t err = send_cmd(pd, &cmd, NULL, 1000);
  if (err) return err;

  return ESUCCESS;
}

static errno_t send_cmd(Device_wifi_bluetooth *const pd, wb_string *cmd, wb_string *rt_data, uint32_t timeout_ms) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  err = pd->usart->ops->transmit(pd->usart, cmd->buf, cmd->len);
  if (err) return err;

  match_fn_t *const match_fns[] = { match_cmd_success, match_cmd_error, match_cmd_unknown };
  err = wait_ack(pd, rt_data, timeout_ms, match_fns, sizeof(match_fns) / sizeof(match_fn_t *));
  if (err) return err;

  return ESUCCESS;
}

static errno_t wait_ack(
  Device_wifi_bluetooth *const pd // wifi 设备
  , wb_string *rt_data
  , uint32_t timeout_ms // 超时时间
  , match_fn_t *const match_fns[]
  , uint8_t match_fn_count
) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  uint32_t begin = 0, now = 0;
  err = pd->timer->ops->get_count(pd->timer, &begin);
  if (err) return err;

  uint8_t buf[WAIT_ACT_BUFFER_SIZE] = {0};
  wb_string bs = { .buf = buf, .len = 0, .size = WAIT_ACT_BUFFER_SIZE };
  uint32_t read_len = 0;

  for (;;) {
    err = pd->timer->ops->get_count(pd->timer, &now);
    if (err) return err;
    if (now - begin >= timeout_ms) return ETIMEDOUT;
    if (bs.len == bs.size) return EOVERFLOW;

    err = pd->usart->ops->receive(pd->usart, bs.buf + bs.len, &read_len, bs.size - bs.len);
    if (err) return err;
    if (read_len == 0) continue; // 没有新数据时，继续等待
    bs.len += read_len;

    bool matched = false;
    Matched_mark mark = MATCHED_MARK_FAIL;

    for (uint8_t i = 0; i < match_fn_count; i++) {
      err = match_fns[i](&bs, &matched, &mark);
      if (err) return err;

      if (!matched) continue;

      if (rt_data) {
        errno_t err = wb_string_concat(rt_data, rt_data);
        if (err) return err;
      }

      switch (mark) {
        case MATCHED_MARK_SUCCESS: return ESUCCESS;
        case MATCHED_MARK_FAIL: return EIO;
        default: return EINVAL;
      }
    }
  }
}

/**
 * @brief 匹配指令执行成功响应信息
 */
errno_t match_cmd_success(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // 指令执行成功标志
  const char *keyword = "\r\nOK\r\n";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 只比较最后几个字符
  const bool flag = strncmp((char *)msg->buf + msg->len - keyword_len, keyword, keyword_len) == 0;
  if (flag) {
    *rt_mark_ptr = MATCHED_MARK_SUCCESS;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

// 匹配指令执行错误响应信息
errno_t match_cmd_error(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // \r\n+<CMD>:<error_code>\r\nERROR\r\n
  // errorno 表示错误码
  const char *keyword = "\r\nERROR\r\n";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 只比较最后几个字符
  const bool flag = strncmp((char *)msg->buf + msg->len - keyword_len, keyword, keyword_len) == 0;
  if (flag) {
    printf("wait_act_cmd_error -> %.*s\r\n", (int)msg->len, msg->buf);
    *rt_mark_ptr = MATCHED_MARK_FAIL;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

// 匹配响应未知指令信息
errno_t match_cmd_unknown(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // Unknown cmd:<串口输入的所有内容，包含参数>
  const char *keyword = "Unknown cmd";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 只比较前几个字符
  const bool flag = strncmp((char *)msg->buf, keyword, keyword_len) == 0;
  if (flag) {
    printf("wait_act_cmd_unknown -> %.*s\r\n", (int)msg->len, msg->buf);
    *rt_mark_ptr = MATCHED_MARK_FAIL;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

errno_t match_event_got_ip(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // +EVENT:WIFI_GOT_IP // 获取到 IP
  const char *keyword = "+EVENT:WIFI_GOT_IP";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 只比较最后几个字符
  const bool flag = strncmp((char *)msg->buf + msg->len - keyword_len, keyword, keyword_len) == 0;
  if (flag) {
    *rt_mark_ptr = MATCHED_MARK_SUCCESS;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

/**
 * @brief 从后向前查找子字符串
 * 
 * @param haystack 主字符串
 * @param needle 子字符串
 * @param rt_matched_ptr 返回是否匹配到字符串 
 * @param rt_idx_ptr 如果匹配到字符串, 返回从右到左匹配到的第一个子字符串的第一个字母的索引 
 * @return 
 */
void rstrstr(const wb_string *haystack, const wb_string *needle, bool *rt_matched_ptr, uint32_t *rt_idx_ptr) {
  *rt_matched_ptr = false;

  if (needle->len == 0 || haystack->len < needle->len) return;

  for (uint32_t i = haystack->len - needle->len + 1; i-- > 0;) {
    if (strncmp((char *)haystack->buf + i, (char *)needle->buf, needle->len) == 0) {
      *rt_matched_ptr = true;
      *rt_idx_ptr = i;
      return;
    }
  }

  return;
}

static errno_t wb_string_concat(wb_string *const aim, const wb_string *const from) {
  if (aim == from) return EINVAL;
  if (from->len == 0) return ESUCCESS;
  if (from->len > aim->size - aim->len) return EOVERFLOW;

  memcpy(aim->buf + aim->len, from->buf, from->len);
  aim->len += from->len;

  return ESUCCESS;
}
