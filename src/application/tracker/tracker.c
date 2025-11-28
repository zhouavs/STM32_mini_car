#include "tracker.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "common/delay/delay.h"
#include "device_config/gpio/gpio.h"
#include "device_config/usart/usart.h"
#include "device_config/timer/timer.h"
#include "device_config/pwm/pwm.h"
#include "device_config/spi/spi.h"
#include "device_config/st7789v2/st7789v2.h"
#include "device_config/motor/motor.h"
#include "device_config/speed_test/speed_test.h"
#include "device_config/tracker/tracker.h"

static errno_t init(void);
static errno_t callback(void);

static Device_timer *pdtimer = NULL;
static Device_motor *pdm_hl = NULL, *pdm_hr = NULL, *pdm_tl = NULL, *pdm_tr = NULL;
static Device_tracker *pdt = NULL;

void tracker_test(void) {
  errno_t err = ESUCCESS;

  printf("tracker_test start\r\n");

  err = init();
  if (err) goto err_tag;

  err = Device_timer_find(&pdtimer, DEVICE_TIMER_TIM6);
  if (err) goto err_tag;
  
  err = pdtimer->ops->set_period_elapsed_callback(pdtimer, callback);
  if (err) goto err_tag;
  err = pdtimer->ops->set_period(pdtimer, 5000);
  if (err) goto err_tag;
  bool timer_running = false;
  err = pdtimer->ops->is_running(pdtimer, &timer_running);
  if (err) goto err_tag;
  if (timer_running == false) {
    err = pdtimer->ops->start(pdtimer, DEVICE_TIMER_START_MODE_IT);
    if (err) goto err_tag;
  }

  err = Device_motor_find(&pdm_hl, DEVICE_MOTOR_HEAD_LEFT);
  if (err) goto err_tag;
  err = Device_motor_find(&pdm_hr, DEVICE_MOTOR_HEAD_RIGHT);
  if (err) goto err_tag;
  err = Device_motor_find(&pdm_tl, DEVICE_MOTOR_TAIL_LEFT);
  if (err) goto err_tag;
  err = Device_motor_find(&pdm_tr, DEVICE_MOTOR_TAIL_RIGHT);
  if (err) goto err_tag;
  
  err = Device_tracker_find(&pdt, DEVICE_TRACKER_1);
  if (err) goto err_tag;
  err = pdt->ops->init(pdt);
  if (err) goto err_tag;

  while (1) {
    // uint8_t center = 0;
    // err = pdt->ops->get_line_center(pdt, &center);
    // if (err) goto err_tag;
    // printf("tracker_value\r\nin line: %d\r\nline center: %d\r\n", (center & 0x80) >> 7, center & 0x7F);
    // delay_s(1);
  }

  err_tag:
  while (1);
}

static errno_t init(void) {
  errno_t err = ESUCCESS;

  err = Device_config_GPIO_register_all_device();
  if (err) return err;
  
  err = Device_config_USART_register_all_device();
  if (err) return err;
  
  err = Device_config_timer_register_all_device();
  if (err) return err;
  
  err = Device_config_PWM_register_all_device();
  if (err) return err;
  
  err = Device_config_SPI_register_all_device();
  if (err) return err;
  
  err = Device_config_ST7789V2_register_all_device();
  if (err) return err;
  
  err = Device_config_motor_register_all_device();
  if (err) return err;
  
  err = Device_config_speed_test_register_all_device();
  if (err) return err;
  
  err = Device_config_tracker_register_all_device();
  if (err) return err;

  return ESUCCESS;
}

static errno_t callback(void) {
  errno_t err = ESUCCESS;

  uint8_t center = 0;
  err = pdt->ops->get_line_center(pdt, &center);
  if (err) return err;

  if ((center & 0x80) == 0) {
    // 未检查到导航线
    err = pdm_hl->ops->stop(pdm_hl);
    if (err) goto cb_err_tag;
    err = pdm_tl->ops->stop(pdm_tl);
    if (err) goto cb_err_tag;
    err = pdm_hr->ops->stop(pdm_hr);
    if (err) goto cb_err_tag;
    err = pdm_tr->ops->stop(pdm_tr);
    if (err) goto cb_err_tag;
  } else {
    switch (center & 0x7F) {
      case 0: {
        // 导航线在最左边
        err = pdm_hl->ops->backward(pdm_hl, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tl->ops->backward(pdm_tl, 0xFF);
        if (err) goto cb_err_tag;

        err = pdm_hr->ops->forward(pdm_hr, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tr->ops->forward(pdm_tr, 0xFF);
        if (err) goto cb_err_tag;

        break;
      }
      case 1: {
        // 导航线在从左往右第二个
        err = pdm_hl->ops->stop(pdm_hl);
        if (err) goto cb_err_tag;
        err = pdm_tl->ops->stop(pdm_tl);
        if (err) goto cb_err_tag;

        err = pdm_hr->ops->forward(pdm_hr, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tr->ops->forward(pdm_tr, 0xFF);
        if (err) goto cb_err_tag;

        break;
      }
      case 2: {
        // 导航线在从左往右第三个
        err = pdm_hl->ops->forward(pdm_hl, 0x80);
        if (err) goto cb_err_tag;
        err = pdm_tl->ops->forward(pdm_tl, 0x80);
        if (err) goto cb_err_tag;

        err = pdm_hr->ops->forward(pdm_hr, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tr->ops->forward(pdm_tr, 0xFF);
        if (err) goto cb_err_tag;

        break;
      }
      case 3: {
        // 导航线在中间
        err = pdm_hl->ops->forward(pdm_hl, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tl->ops->forward(pdm_tl, 0xFF);
        if (err) goto cb_err_tag;

        err = pdm_hr->ops->forward(pdm_hr, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tr->ops->forward(pdm_tr, 0xFF);
        if (err) goto cb_err_tag;

        break;
      }
      case 4: {
        // 导航线在从右往左第三个
        err = pdm_hl->ops->forward(pdm_hl, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tl->ops->forward(pdm_tl, 0xFF);
        if (err) goto cb_err_tag;

        err = pdm_hr->ops->forward(pdm_hr, 0x80);
        if (err) goto cb_err_tag;
        err = pdm_tr->ops->forward(pdm_tr, 0x80);
        if (err) goto cb_err_tag;

        break;
      }
      case 5: {
        // 导航线在从右往左第二个
        err = pdm_hl->ops->forward(pdm_hl, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tl->ops->forward(pdm_tl, 0xFF);
        if (err) goto cb_err_tag;

        err = pdm_hr->ops->stop(pdm_hr);
        if (err) goto cb_err_tag;
        err = pdm_tr->ops->stop(pdm_tr);
        if (err) goto cb_err_tag;

        break;
      }
      case 6: {
        // 导航线在最右边
        err = pdm_hl->ops->forward(pdm_hl, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tl->ops->forward(pdm_tl, 0xFF);
        if (err) goto cb_err_tag;

        err = pdm_hr->ops->backward(pdm_hr, 0xFF);
        if (err) goto cb_err_tag;
        err = pdm_tr->ops->backward(pdm_tr, 0xFF);
        if (err) goto cb_err_tag;

        break;
      }
      default: {
        goto cb_err_tag;
      }
    }
  }

  return ESUCCESS;

  cb_err_tag:
  err = pdm_hl->ops->stop(pdm_hl);
  err = pdm_tl->ops->stop(pdm_tl);
  err = pdm_hr->ops->stop(pdm_hr);
  err = pdm_tr->ops->stop(pdm_tr);
  return EIO;
}
