#ifndef __LSSVC_FILEMGR_H__
#define __LSSVC_FILEMGR_H__

#include "lssvc_filelog.h"
#include "lssvc_singleton.h"
#include "noncopyable.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

// @brief global LSSFileMgr object
#define g_file_mgr lssvc::utils::LSSSingleton<lssvc::utils::LSSFileMgr>::getInstance()

namespace lssvc::utils {

class LSSFileMgr : public NonCopyable {
public:
  LSSFileMgr() = default;
  ~LSSFileMgr() = default;

  // @brief update time data & maybe do the rotations
  void update();

  // @brief get the file logger searching by the input filename
  LSSFileLogPtr getFileLog(const std::string &filename);

  // @brief remove the log file
  void removeFileLog(const LSSFileLogPtr &log);

  void rotateDays(const LSSFileLogPtr &file);
  void rotateHours(const LSSFileLogPtr &file);
  void rotateMinutes(const LSSFileLogPtr &file);

private:
  // first: log file name, second: pointer to the log file
  std::unordered_map<std::string, LSSFileLogPtr> logs_;

  std::mutex lock_; // mutex for altering 'logs_'

  // the following member-variables are used for comparing between the current time and the stored time,
  // and then update log file according to 'RotateType' type.
  int last_minute_{-1};
  int last_hour_{-1};
  int last_month_{-1};
  int last_day_{-1};
  int last_year_{-1};

};

} // namespace lssvc::utils

#endif
