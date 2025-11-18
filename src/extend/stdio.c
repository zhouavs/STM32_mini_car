#include <stdio.h>
#include "device/usart/usart.h"

int __io_putchar(int ch) {
  Device_USART *pd = NULL;
  errno_t err = Device_USART_find(&pd, DEVICE_USART_DEBUG);
  if (err) return -1;

  uint8_t c = (uint8_t)ch;
  err = pd->ops->transmit(pd, &c, 1);
  if (err) return -1;

  return ch;
}

int _write(int fd, const char *buf, int len) {
  if (!(fd == 1 || fd == 2)) return -1;

  Device_USART *pd = NULL;
  errno_t err = Device_USART_find(&pd, DEVICE_USART_DEBUG);
  if (err) return -1;

  for (int i = 0; i < len; ++i) {
    if (__io_putchar(buf[i]) < 0) return -1;
  }

  return len;
}

struct __FILE{
    int handle;
};

FILE __stdout;

int fputc(int ch, FILE *f)
{
    (void)f;
    Device_USART *pd = NULL;
    errno_t err = Device_USART_find(&pd, DEVICE_USART_DEBUG);
    if (err) return 0;
    
    if(pd->ops->transmit(pd, (unsigned char*)&ch, 1) == 1)
        return ch;
    
    return 0;
}

