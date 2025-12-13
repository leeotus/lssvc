#ifndef __LSSVC_TIME_H__
#define __LSSVC_TIME_H__

#include <string>

namespace lssvc::utils {

class LSSTime {
public:
  LSSTime() = delete;
  ~LSSTime() = delete;

  static int64_t nowMs();
  static int64_t now();

  /**
   * @brief get the timestamp and then translate into localtime (and store
   * the data into the params)
   *
   * @param year [out]
   * @param month [out]
   * @param day [out]
   * @param hour [out]
   * @param minute [out]
   * @param second [out]
   * @return int64_t return the timestamp
   */
  static int64_t now(int &year, int &month, int &day, int &hour, int &minute, int &second);

  static std::string getISOTime();
};

} // namepace lssvc::utils

#endif
