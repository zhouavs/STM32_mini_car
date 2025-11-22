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

  // 定时器在 start 中会重置计数为 0, 不需要人为 +1; 之前的 +1 会造成多延时一周期 (例如 1 秒变 2 秒)

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
  if (err) return err;

  err = pdt->ops->start(pdt);
  if (err) return err;

  err = pdt->ops->get_count(pdt, &count);
  if (err) return err;
  
  // 计算目标计数值 (当前计数 + 期望周期数), 此处当前计数即 0
  if (aim_count + count < count) return EINVAL; // 溢出保护
  uint32_t target = count + aim_count;
  
  do {  
    err = pdt->ops->get_count(pdt, &count);
    if (err) return err;
  } while (count < target);

  err = pdt->ops->stop(pdt);
  if (err) return err;

  return ESUCCESS;
}
