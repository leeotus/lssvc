#ifndef __LSSVC_FILE_LOG_H__
#define __LSSVC_FILE_LOG_H__

#include <string>
#include <memory>

namespace lssvc::utils {

class LSSFileLog;

// @note smart pointer, will be stored in the 'unordered_map'
using LSSFileLogPtr = std::shared_ptr<LSSFileLog>;

// rotate modes, indicate which mode a logfile should use,
// the corresponding logfile will be updated when meet the conditions.
enum RotateType {
  kRotateNone,
  kRotateMinute,
  kRotateHour,
  kRotateDay,
};

class LSSFileLog {
public:
  LSSFileLog() = default;
  ~LSSFileLog() = default;

  bool open(const std::string &filepath);
  size_t writeLog(const std::string &msg);
  void rotate(const std::string &file);
  void setRotate(RotateType type);
  RotateType getRotateType() const;
  int64_t fileSize() const;
  std::string filePath() const;

private:
  int fd_{-1};
  std::string file_path_;
  RotateType rotate_type_{kRotateNone};
};

} // namespace lssvc::utils

#endif
