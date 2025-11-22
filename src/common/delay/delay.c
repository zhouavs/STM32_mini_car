#include "delay.h"
#include "stm32f4xx_hal.h"
#include "device/timer/timer.h"

static errno_t delay(uint32_t period_us_num, uint32_t aim_count);

errno_t delay_s(uint32_t s) {
  // 只有使用 TIM2 这种有 32 位计数器的定时器才能按秒计数, 否则 period_us_num 无法设定为 1_000_000
  return delay(1000000, s);
}

errno_t delay_ms(uint32_t ms) {
  // HAL_Delay(ms);
  return delay(1000, ms);
}

errno_t delay_us(uint32_t us) {
  return delay(1, us);
}

/**
 * @brief 延时
 * 使用 TIM2 定时器(因为 TIM2 的计数寄存器有 32 位), 需要先初始化 timer 模块并注册 timer
 * @param period_us_num 每周期有多少微秒
 * @param aim_count 目标计数量
 * @return 错误信息
 */
static errno_t delay(uint32_t period_us_num, uint32_t aim_count) {
  const Device_timer *pdt = NULL;
  errno_t err = ESUCCESS;

  // 至少计数设定微秒数, 防止调用延时函数时正处在定时器计数末期, 导致实际定时时长小于设置微秒数
  if (aim_count < 0xFFFFFFFF) ++aim_count;

  err = Device_timer_find(&pdt, DEVICE_TIMER_TIM2);
  if (err) return err;

  bool running = false;
  err = pdt->ops->is_running(pdt, &running);
  if (err) return err;
  if (running) {
    err = pdt->ops->stop(pdt);
    if (err) return err;
  }

  uint32_t count = 0;

  err = pdt->ops->set_preiod(pdt, period_us_num);
  err = pdt->ops->start(pdt);

  err = pdt->ops->get_count(pdt, &count);
  if (err) return err;
  
  if (aim_count + count < aim_count) {
    // 溢出, 超出定时器计数范围
    return EINVAL;
  }
  aim_count += count;
  
  do {  
    err = pdt->ops->get_count(pdt, &count);
    if (err) return err;
  } while (count < aim_count);

  err = pdt->ops->stop(pdt);
  if (err) return err;

  return ESUCCESS;
}
