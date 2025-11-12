#include <stdlib.h>
#include <stdio.h>
#include "usart.h"
#include "device/usart/usart.h"
#include "device_config/usart/usart.h"

void usart_test(void) {
  errno_t err = Device_USART_module_init();
  if (err) return;

  err = Device_config_USART_register_all_device();
  if (err) return;

  Device_USART *pdu = NULL;
  err = Device_USART_find(&pdu, DEVICE_USART_DEBUG);
  if (err) return;

  err = pdu->ops->init(pdu);
  if (err) return;

  setvbuf(stdout, NULL, _IONBF, 0);
  printf("USART printf read\r\n");

  while (1) {
    uint8_t rx_data[1] = { 0 };
    uint32_t rx_data_len = 0;
    err = pdu->ops->receive(pdu, rx_data, &rx_data_len, 1);
    if (err || rx_data_len == 0) continue;
    rx_data[0]++;
    pdu->ops->transmit(pdu, rx_data, 1);
  }
}
