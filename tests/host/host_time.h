#pragma once
#include <ctime>
namespace esphome {
struct HostLocalTime {
  unsigned year, month, day_of_month, day_of_week;
};
inline HostLocalTime host_local_time(time_t epoch) {
  tm local{};
  localtime_r(&epoch, &local);
  return {static_cast<unsigned>(local.tm_year + 1900), static_cast<unsigned>(local.tm_mon + 1),
          static_cast<unsigned>(local.tm_mday), static_cast<unsigned>(local.tm_wday + 1)};
}
}
