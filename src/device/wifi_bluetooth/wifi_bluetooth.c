#include "wifi_bluetooth.h"
#include "common/list/list.h"
#include "common/str_to_num/str_to_num.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

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

/**
 * @brief 端口号和链接ID关联关系
 * [0] 表示是否正在使用 [1] 表示端口号 [2] 表示链接ID
 */
typedef uint32_t port_con_id_relate_t[3];

// 匹配等待响应的函数
typedef errno_t (match_fn_t)(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);

// 对象方法
static errno_t init(Device_wifi_bluetooth *const pd);
static errno_t join_wifi_ap(Device_wifi_bluetooth *const pd, const uint8_t *const ssid, const uint8_t *const pwd);
static errno_t create_socket_connection(
  Device_wifi_bluetooth *const pd
  , Device_wifi_bluetooth_socket_type type
  , uint8_t *remote_host
  , uint16_t port
);
static errno_t delete_socket_connection(Device_wifi_bluetooth *const pd, uint32_t port);
static errno_t socket_send(Device_wifi_bluetooth *const pd, uint32_t port, uint8_t *data_buf, uint32_t data_len);

// 内部方法
// 配置接收方式
static errno_t socket_receive_config(Device_wifi_bluetooth *const pd, Device_wifi_bluetooth_socket_receive_mode mode);
// 等待直到收到特定的响应信息
static errno_t wait_ack(
  Device_wifi_bluetooth *const pd // wifi 设备
  , wb_string *rt_data
  , uint32_t timeout_ms // 超时时间
  , match_fn_t *const match_fns[]
  , uint8_t match_fn_count
);
// 匹配指令执行成功响应信息
static errno_t match_ok(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 匹配指令执行错误响应信息
static errno_t match_error(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 匹配响应未知指令信息
static errno_t match_cmd_unknown(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 匹配获取到 IP 事件
static errno_t match_event_got_ip(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 匹配获取到可以开始发送数据的 > 符号信息
static errno_t match_socket_send_start(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 匹配 +SOCKETREAD:<ConID>,<len>,<data> 中的 +SOCKETREAD:
static errno_t match_socket_read_start(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr);
// 新增、删除、查找接口和链接ID关联
static errno_t port_con_id_relate_add(uint32_t port, uint32_t con_id);
static errno_t port_con_id_relate_del(uint32_t port);
static errno_t port_con_id_relate_find(uint32_t port, bool *rt_exist_ptr, uint32_t *rt_con_id_ptr);
// 从后向前匹配字符串
void rstrstr(const wb_string *haystack, const wb_string *needle, bool *rt_matched_ptr, uint32_t *rt_idx_ptr);
// 查找设备
static inline uint8_t match_device_by_name(const void *const name, const void *const pd);

static const Device_wifi_bluetooth_ops device_ops = {
  .init = init,
  .join_wifi_ap = join_wifi_ap,
  .create_socket_connection = create_socket_connection,
  .delete_socket_connection = delete_socket_connection,
  .socket_send = socket_send,
};

static List *list = NULL;
#define PORT_CON_ID_RELATE_SIZE 10
static port_con_id_relate_t port_con_id_relates[PORT_CON_ID_RELATE_SIZE] = {0};
static uint8_t port_con_id_relate_count = 0;


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

  err = socket_receive_config(pd, RECEIVE_MODE_PASSIVE);

  return ESUCCESS;
}

static errno_t join_wifi_ap(Device_wifi_bluetooth *const pd, const uint8_t *const ssid, const uint8_t *const pwd) {
  if (pd == NULL) return EINVAL;

  uint8_t cmd_buf[100] = {0};
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 100 - 1 };

  cmd.len = snprintf((char *)cmd_buf, cmd.size, "AT+WJAP=%s,%s\r\n", ssid, pwd);
  if (cmd.len > cmd.size) {
    return EOVERFLOW;
  }

  errno_t err = ESUCCESS;

  err = pd->usart->ops->clear_receive_buf(pd->usart);
  if (err) return err;

  err = pd->usart->ops->transmit(pd->usart, cmd.buf, cmd.len);
  if (err) return err;

  match_fn_t *const match_cmd_fns[] = { match_ok, match_error, match_cmd_unknown };
  err = wait_ack(pd, NULL, 1000, match_cmd_fns, sizeof(match_cmd_fns) / sizeof(match_fn_t *));
  if (err) return err;

  match_fn_t *const match_event_fns[] = { match_event_got_ip };
  err = wait_ack(pd, NULL, 5000, match_event_fns, sizeof(match_event_fns) / sizeof(match_fn_t *));
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
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 50 - 1 };

  switch (type) {
    case UDP_CLIENT:
    case TCP_CLIENT:
    case SSL_CLIENT: {
      if (remote_host == NULL) return EINVAL;
      cmd.len = snprintf((char *)cmd_buf, cmd.size, "AT+SOCKET=%d,%s,%d\r\n", type, remote_host, port);
      break;
    }
    case UDP_SERVER:
    case TCP_SERVER:
    case SSL_SERVER: {
      cmd.len = snprintf((char *)cmd_buf, cmd.size, "AT+SOCKET=%d,%d\r\n", type, port);
      break;
    }
    default: {
      return EINVAL;
    }
  }

  if (cmd.len > cmd.size) return EOVERFLOW;

  uint8_t data_buf[50] = {0};
  wb_string data = { .buf = data_buf, .len = 0, .size = 50 - 1 };

  err = pd->usart->ops->clear_receive_buf(pd->usart);
  if (err) return err;

  err = pd->usart->ops->transmit(pd->usart, cmd.buf, cmd.len);
  if (err) return err;

  match_fn_t *const match_fns[] = { match_ok, match_error, match_cmd_unknown };
  err = wait_ack(pd, &data, 5000, match_fns, sizeof(match_fns) / sizeof(match_fn_t *));
  if (err) return err;

  const char *pre_str = "connect success ConID=";
  const uint8_t pre_str_len = strlen(pre_str);

  if (data.len > data.size) return EOVERFLOW;

  char *con_id_start = strstr((char *)data.buf, pre_str);
  if (con_id_start == NULL) return EIO;
  con_id_start += pre_str_len;

  uint32_t con_id = 0;
  bool trans_fail = str_to_uint32(con_id_start, &con_id, NULL);
  if (trans_fail) return EIO;

  // 关联端口和链接ID
  err = port_con_id_relate_add(port, con_id);
  if (err) return err;

  return ESUCCESS;
}

static errno_t delete_socket_connection(Device_wifi_bluetooth *const pd, uint32_t port) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  // 根据端口找链接ID, 未找到直接返回删除成功
  bool con_id_exist = false;
  uint32_t con_id = 0;
  err = port_con_id_relate_find(port, &con_id_exist, &con_id);
  if (err) return err;
  if (!con_id_exist) return ESUCCESS;

  uint8_t cmd_buf[26] = {0};
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 26 - 1 };

  cmd.len = snprintf((char *)cmd.buf, cmd.size, "AT+SOCKETDEL=%d\r\n", con_id);

  if (cmd.len > cmd.size) {
    return EOVERFLOW;
  }

  err = pd->usart->ops->clear_receive_buf(pd->usart);
  if (err) return err;

  err = pd->usart->ops->transmit(pd->usart, cmd.buf, cmd.len);
  if (err) return err;

  match_fn_t *const match_fns[] = { match_ok, match_error, match_cmd_unknown };
  err = wait_ack(pd, NULL, 1000, match_fns, sizeof(match_fns) / sizeof(match_fn_t *));
  if (err) return err;

  return ESUCCESS;
}

static errno_t socket_send(Device_wifi_bluetooth *const pd, uint32_t port, uint8_t *data_buf, uint32_t data_len) {
  if (pd == NULL || data_buf == NULL || data_len == 0) return EINVAL;

  errno_t err = ESUCCESS;

  // 根据端口找链接ID, 未找到报错
  bool con_id_exist = false;
  uint32_t con_id = 0;
  err = port_con_id_relate_find(port, &con_id_exist, &con_id);
  if (err) return err;
  if (!con_id_exist) return ENOTCONN;

  uint8_t cmd_buf[33] = {0};
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 33 - 1 };

  cmd.len = snprintf((char *)cmd.buf, cmd.size, "AT+SOCKETSEND=%d,%d\r\n", con_id, data_len);
  if (cmd.len > cmd.size) return EOVERFLOW;

  err = pd->usart->ops->clear_receive_buf(pd->usart);
  if (err) return err;

  err = pd->usart->ops->transmit(pd->usart, cmd.buf, cmd.len);
  if (err) return err;

  match_fn_t *const match_start_fns[] = { match_socket_send_start, match_error, match_cmd_unknown };
  err = wait_ack(pd, NULL, 1000, match_start_fns, sizeof(match_start_fns) / sizeof(match_fn_t *));
  if (err) return err;

  wb_string data = { .buf = data_buf, .len = data_len, .size = data_len };
  err = pd->usart->ops->transmit(pd->usart, data.buf, data.len);
  if (err) return err;

  match_fn_t *const match_ok_fns[] = { match_ok, match_error, match_cmd_unknown };
  err = wait_ack(pd, NULL, 1000, match_ok_fns, sizeof(match_ok_fns) / sizeof(match_fn_t *));
  if (err) return err;

  return ESUCCESS;
}

static errno_t socket_read(Device_wifi_bluetooth *const pd, uint32_t port, uint8_t *rt_data_ptr, uint32_t *rt_data_len_ptr, uint32_t data_size) {
  if (pd == NULL) return EINVAL;

  errno_t err = ESUCCESS;

  bool con_id_exist = false;
  uint32_t con_id = 0;
  err = port_con_id_relate_find(port, &con_id_exist, &con_id);
  if (err) return err;
  if (!con_id_exist) return ENOTCONN;

  uint8_t cmd_buf[26] = {0};
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 26 - 1 };
  cmd.len = snprintf((char *)cmd.buf, cmd.size, "AT+SOCKETREAD=%d\r\n", con_id);
  if (cmd.len > cmd.size) return EOVERFLOW;

  err = pd->usart->ops->clear_receive_buf(pd->usart);
  if (err) return err;

  err = pd->usart->ops->transmit(pd->usart, cmd.buf, cmd.len);
  if (err) return err;

  // 直接使用外部 buf 作为容器
  wb_string data_str = { .buf = rt_data_ptr, .len = 0, .size = data_size - 1 };

  // 等待 +SOCKETREAD:
  match_fn_t *const match_start_fns[] = { match_socket_read_start, match_error, match_cmd_unknown };
  err = wait_ack(pd, &data_str, 1000, match_start_fns, sizeof(match_start_fns) / sizeof(match_fn_t *));
  if (err) return err;

  // 等待 OK
  match_fn_t *const match_ok_fns[] = { match_ok, match_error, match_cmd_unknown };
  err = wait_ack(pd, &data_str, 1000, match_ok_fns, sizeof(match_ok_fns) / sizeof(match_fn_t *));
  if (err) return err;

  // 解析 data_str 中的数据, 解析 +SOCKETREAD:<ConID>,<len>,<data>
  const char *pre_str = "+SOCKETREAD:";
  const uint8_t pre_str_len = strlen(pre_str);

  uint32_t read_con_id = 0, read_len = 0;
  bool trans_fail = false;

  // 指针指向 conID 对应的字符起始位, 跳过 "+SOCKETREAD:
  uint8_t *data_buf_ptr = (uint8_t *)strstr((char *)data_str.buf, pre_str);
  if (data_buf_ptr == NULL) return EIO;
  data_buf_ptr += pre_str_len;
  // conID 字符串转换为数字
  trans_fail = str_to_uint32((char *)data_buf_ptr, &read_con_id, (char **)&data_buf_ptr);
  if (trans_fail) return EIO;

  // 指针指向 len 对应字符起始位, 跳过 ,
  data_buf_ptr += 1;
  // len 字符串转换为数字
  trans_fail = str_to_uint32((char *)data_buf_ptr, &read_len, (char **)&data_buf_ptr);
  if (trans_fail) return EIO;

  // 指针指向 data 字符起始位
  data_buf_ptr += 1;
  // 把后续 len 个字节移动到外部 buf 的首位
  memmove(data_str.buf, data_buf_ptr, read_len);
  // 后续字符设置为0
  memset(data_buf_ptr + read_len, 0, data_str.size - read_len);

  *rt_data_len_ptr = read_len;

  return ESUCCESS;
}

static errno_t socket_receive_config(Device_wifi_bluetooth *const pd, Device_wifi_bluetooth_socket_receive_mode mode) {
  if (pd == NULL) return NULL;

  uint8_t cmd_buf[25] = {0};
  wb_string cmd = { .buf = cmd_buf, .len = 0, .size = 25 - 1 };
  cmd.len = snprintf((char *)cmd.buf, cmd.size, "AT+SOCKETRECVCFG=%d\r\n", mode);
  if (cmd.len > cmd.size) return EOVERFLOW;

  errno_t err = ESUCCESS;

  err = pd->usart->ops->clear_receive_buf(pd->usart);
  if (err) return err;

  err = pd->usart->ops->transmit(pd->usart, cmd.buf, cmd.len);
  if (err) return err;

  match_fn_t *const match_fns[] = { match_ok, match_error, match_cmd_unknown };
  err = wait_ack(pd, NULL, 1000, match_fns, sizeof(match_fns) / sizeof(match_fn_t *));
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

  uint8_t buf[100] = {0};
  wb_string str = { .buf = buf, .len = 0, .size = 100 - 1 };

  // 如果有外部传入的字符串, 使用外部的, 否则采用当前函数创建的
  wb_string *str_ptr = rt_data != NULL ? rt_data : &str;

  uint32_t read_len = 0;

  for (;;) {
    if (str_ptr->len == str_ptr->size) return EOVERFLOW;

    err = pd->timer->ops->get_count(pd->timer, &now);
    if (err) return err;
    if (now - begin >= timeout_ms) return ETIMEDOUT;

    err = pd->usart->ops->receive(pd->usart, str_ptr->buf + str_ptr->len, &read_len, str_ptr->size - str_ptr->len);
    if (err) return err;
    if (read_len == 0) continue; // 没有新数据时，继续等待
    str_ptr->len += read_len;

    bool matched = false;
    Matched_mark mark = MATCHED_MARK_FAIL;

    for (uint8_t i = 0; i < match_fn_count; i++) {
      err = match_fns[i](&str, &matched, &mark);
      if (err) return err;

      if (!matched) continue;

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
static errno_t match_ok(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // 指令执行成功标志
  const char *keyword = "\r\nOK\r\n";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 比较最后几个字符
  const bool flag = strncmp((char *)msg->buf + msg->len - keyword_len, keyword, keyword_len) == 0;
  if (flag) {
    *rt_mark_ptr = MATCHED_MARK_SUCCESS;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

// 匹配指令执行错误响应信息
static errno_t match_error(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // \r\n+<CMD>:<error_code>\r\nERROR\r\n
  // errorno 表示错误码
  const char *keyword = "\r\nERROR\r\n";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 比较最后几个字符
  const bool flag = strncmp((char *)msg->buf + msg->len - keyword_len, keyword, keyword_len) == 0;
  if (flag) {
    printf("wait_act_cmd_error -> %.*s\r\n", (int)msg->len, msg->buf);
    *rt_mark_ptr = MATCHED_MARK_FAIL;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

// 匹配响应未知指令信息
static errno_t match_cmd_unknown(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // Unknown cmd:<串口输入的所有内容，包含参数>
  const char *keyword = "Unknown cmd";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 比较前几个字符
  const bool flag = strncmp((char *)msg->buf, keyword, keyword_len) == 0;
  if (flag) {
    printf("wait_act_cmd_unknown -> %.*s\r\n", (int)msg->len, msg->buf);
    *rt_mark_ptr = MATCHED_MARK_FAIL;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

static errno_t match_event_got_ip(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // +EVENT:WIFI_GOT_IP // 获取到 IP
  const char *keyword = "+EVENT:WIFI_GOT_IP";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 比较前几个字符
  const bool flag = strncmp((char *)msg->buf, keyword, keyword_len) == 0;
  if (flag) {
    *rt_mark_ptr = MATCHED_MARK_SUCCESS;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

static errno_t match_socket_send_start(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // > 可以开始发送数据
  if (msg->len == 0) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 比较第一个字符
  const bool flag = msg->buf[0] == '>';
  if (flag) {
    *rt_mark_ptr = MATCHED_MARK_SUCCESS;
  }

  *rt_matched_ptr = flag;

  return ESUCCESS;
}

static errno_t match_socket_read_start(wb_string *msg, bool *rt_matched_ptr, Matched_mark *rt_mark_ptr) {
  // 匹配 +SOCKETREAD:<ConID>,<len>,<data> 中的 +SOCKETREAD:
  // 以便确认可以开始接收信息
  const char *keyword = "+SOCKETREAD:";
  const uint8_t keyword_len = strlen(keyword);

  if (msg->len < keyword_len) {
    *rt_matched_ptr = false;
    return ESUCCESS;
  }

  // 比较前几个字符
  const bool flag = strncmp((char *)msg->buf, keyword, keyword_len) == 0;
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

static errno_t port_con_id_relate_add(uint32_t port, uint32_t con_id) {
  if (port_con_id_relate_count == PORT_CON_ID_RELATE_SIZE) return EOVERFLOW;

  int8_t free_row = -1;

  for (uint8_t i = 0; i < PORT_CON_ID_RELATE_SIZE; i++) {
    if (port_con_id_relates[i][0] == 1) {
      // 该行已被占用
      if (port_con_id_relates[i][1] == port) return EEXIST;
    } else {
      // 该行为空
      if (free_row == -1) free_row = i;
    }
  }

  if (free_row == -1) return EOVERFLOW;

  port_con_id_relates[free_row][0] = 1;
  port_con_id_relates[free_row][1] = port;
  port_con_id_relates[free_row][2] = con_id;
  port_con_id_relate_count++;

  return ESUCCESS;
}

static errno_t port_con_id_relate_del(uint32_t port) {
  if (port_con_id_relate_count == 0) return ESUCCESS;
  
  for (uint8_t i = 0; i < PORT_CON_ID_RELATE_SIZE; i++) {
    if (port_con_id_relates[i][0] == 0 || port_con_id_relates[i][1] != port) continue;

    port_con_id_relates[i][0] = 0;
    port_con_id_relates[i][1] = 0;
    port_con_id_relates[i][2] = 0;
    port_con_id_relate_count--;
  }

  return ESUCCESS;
}

static errno_t port_con_id_relate_find(uint32_t port, bool *rt_exist_ptr, uint32_t *rt_con_id_ptr) {
  if (port_con_id_relate_count == 0) {
    *rt_exist_ptr = false;
    return ESUCCESS;
  }

  for (uint8_t i = 0; i < PORT_CON_ID_RELATE_SIZE; i++) {
    if (port_con_id_relates[i][0] == 0 || port_con_id_relates[i][1] != port) continue;

    *rt_exist_ptr = true;
    *rt_con_id_ptr = port_con_id_relates[i][2];

    return ESUCCESS;
  }

  *rt_exist_ptr = false;
  return ESUCCESS;
}
