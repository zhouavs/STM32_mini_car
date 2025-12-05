#include "str_to_num.h"
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 字符串转 uint32_t
 * 返回值表示转换是否失败
 * @param str 需要转换的字符串
 * @param rt_num_ptr 承接转换成功后返回的数字
 * @param str_num_end_ptr 承接转换成功后返回的该数字后首个非数字字符位置, 可传入 NULL
 * @return true 转换失败
 * @return false 转换成功
 */
bool str_to_uint32(char *str, uint32_t *rt_num_ptr, char **str_num_end_ptr) {
  if (str == NULL || rt_num_ptr == NULL) return true;
  
  char *end_ptr = NULL;
  errno = 0;
  const uint32_t num = strtoul(str, &end_ptr, 10);
  // 没有可转换为数字的子字符串
  if (end_ptr == str) return true;
  // 转换溢出
  if (errno == ERANGE) return true;

  *rt_num_ptr = num;
  if (str_num_end_ptr != NULL) *str_num_end_ptr = end_ptr;

  return false;
}
