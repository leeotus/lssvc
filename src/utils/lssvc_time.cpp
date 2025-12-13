#include "utils/lssvc_time.h"
#include <sys/time.h>   // struct timeval, tm

using namespace lssvc::utils;

int64_t LSSTime::nowMs()
{
  struct timeval tv;
  gettimeofday(&tv, nullptr); // user-mode function
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;    // ms
}

int64_t LSSTime::now()
{
  struct timeval tv;
  gettimeofday(&tv, nullptr);
    return tv.tv_sec; // return only the second part, ignore the usec.
}

int64_t LSSTime::now(int &year, int &month, int &day, int &hour, int &minute, int &second)
{
  struct tm tm;
  time_t t = time(nullptr);   // timestamp
  localtime_r(&t, &tm);       // transfer into local time (thread-safely)

  year = tm.tm_year + 1900;   // from 1900
  month = tm.tm_mon + 1;      // index starts from 0
  day = tm.tm_mday;
  hour = tm.tm_hour;
  minute = tm.tm_min;
  second = tm.tm_sec;
  return t;
}

std::string LSSTime::getISOTime()
{
  struct timeval tv;
  struct tm tm;
  gettimeofday(&tv, nullptr);
  time_t t = time(nullptr);
  localtime_r(&t, &tm);
  char buf[128] = {0};
  auto n = sprintf(buf, "%4d-%02d-%02dT%02d:%02d:%02d", tm.tm_year + 1900,
                   tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return std::string{buf};
}
