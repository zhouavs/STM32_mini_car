#include "rtc.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "Core/Inc/rtc.h"
#include "device/rtc/rtc.h"

#define USED_RTC_FORMAT RTC_FORMAT_BIN

static errno_t get_date_time(Device_RTC *const pd, Device_RTC_date_time *rt_dt_ptr);
static errno_t set_date_time(Device_RTC *const pd, Device_RTC_date_time *dt_ptr);
static errno_t get_bkp_dr(Device_RTC *const pd, Device_RTC_DR_name dr_name, uint32_t *rt_data_ptr);
static errno_t set_bkp_dr(Device_RTC *const pd, Device_RTC_DR_name dr_name, uint32_t data);

static const Driver_RTC_ops ops = {
  .get_date_time = get_date_time,
  .set_date_time = set_date_time,
  .get_bkp_dr = get_bkp_dr,
  .set_bkp_dr = set_bkp_dr,
};

static const uint32_t dr_name_relate_dr[DEVICE_RTC_DR_COUNT] = {
  [DEVICE_RTC_DR_0] = RTC_BKP_DR0,
  [DEVICE_RTC_DR_1] = RTC_BKP_DR1,
  [DEVICE_RTC_DR_2] = RTC_BKP_DR2,
};

errno_t Driver_RTC_get_ops(const Driver_RTC_ops **po_ptr) {
  *po_ptr = &ops;
  return ESUCCESS;
}

static errno_t get_date_time(Device_RTC *const pd, Device_RTC_date_time *rt_dt_ptr) {
  if (pd == NULL || rt_dt_ptr == NULL) return EINVAL;

  RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)pd->instance;
  HAL_StatusTypeDef status = HAL_OK;

  RTC_DateTypeDef date = {0};
  status = HAL_RTC_GetDate(hrtc, &date, USED_RTC_FORMAT);
  if (status != HAL_OK) return EINTR;

  RTC_TimeTypeDef time = {0};
  status = HAL_RTC_GetTime(hrtc, &time, USED_RTC_FORMAT);
  if (status != HAL_OK) return EINTR;

  rt_dt_ptr->date.year = date.Year;
  rt_dt_ptr->date.month = date.Month;
  rt_dt_ptr->date.day = date.Date;
  rt_dt_ptr->date.weekday = date.WeekDay;

  rt_dt_ptr->time.hour = time.Hours;
  rt_dt_ptr->time.minute = time.Minutes;
  rt_dt_ptr->time.second = time.Seconds;

  return ESUCCESS;
}

static errno_t set_date_time(Device_RTC *const pd, Device_RTC_date_time *dt_ptr) {
  if (pd == NULL || dt_ptr == NULL) return EINVAL;

  RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)pd->instance;
  HAL_StatusTypeDef status = HAL_OK;

  RTC_DateTypeDef date = {
    .Year = dt_ptr->date.year,
    .Month = dt_ptr->date.month,
    .Date = dt_ptr->date.day,
    .WeekDay = dt_ptr->date.weekday,
  };
  RTC_TimeTypeDef time = {
    .Hours = dt_ptr->time.hour,
    .Minutes = dt_ptr->time.minute,
    .Seconds = dt_ptr->time.second,
  };

  status = HAL_RTC_SetDate(hrtc, &date, USED_RTC_FORMAT);
  if (status != HAL_OK) return EINTR;
  status = HAL_RTC_SetTime(hrtc, &time, USED_RTC_FORMAT);
  if (status != HAL_OK) return EINTR;

  return ESUCCESS;
}

static errno_t get_bkp_dr(Device_RTC *const pd, Device_RTC_DR_name dr_name, uint32_t *rt_data_ptr) {
  if (pd == NULL || rt_data_ptr == NULL) return EINVAL;

  RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)pd->instance;

  uint32_t dr = dr_name_relate_dr[dr_name];
  *rt_data_ptr = HAL_RTCEx_BKUPRead(hrtc, dr);

  return ESUCCESS;
}

static errno_t set_bkp_dr(Device_RTC *const pd, Device_RTC_DR_name dr_name, uint32_t data) {
  if (pd == NULL) return EINVAL;

  RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)pd->instance;

  uint32_t dr = dr_name_relate_dr[dr_name];
  HAL_RTCEx_BKUPWrite(hrtc, dr, data);

  return ESUCCESS;
}
