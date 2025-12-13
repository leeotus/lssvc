#include "utils/lssvc_filemgr.h"
#include "utils/lssvc_fileutils.h"
#include "utils/lssvc_string.h"
#include "utils/lssvc_time.h"

#include <sstream>

using namespace lssvc::utils;

void LSSFileMgr::rotateDays(const LSSFileLogPtr &file) {
  if (file->fileSize() > 0) {
    char buf[128] = {
        0,
    };
    sprintf(buf, "_%04d-%02d-%02d", last_year_, last_month_, last_day_);
    std::string file_path = file->filePath();
    std::string path = LSSFileUtils::getFilePath(file_path);
    std::string file_name = LSSFileUtils::getFileName(file_path);
    std::string file_ext = LSSFileUtils::getFileExtension(file_path);

    std::ostringstream ss;
    ss << path << file_name << buf << "." << file_ext;
    file->rotate(ss.str());
  }
}

void LSSFileMgr::rotateHours(const LSSFileLogPtr &file) {
  if (file->fileSize() > 0) {
    char buf[128] = {
        0,
    };
    sprintf(buf, "_%04d-%02d-%02dT%02d", last_year_, last_month_, last_day_,
            last_hour_);
    std::string file_path = file->filePath();
    std::string path = LSSFileUtils::getFilePath(file_path);
    std::string file_name = LSSFileUtils::getFileName(file_path);
    std::string file_ext = LSSFileUtils::getFileExtension(file_path);

    std::ostringstream ss;
    ss << path << file_name << buf << "." << file_ext;
    file->rotate(ss.str());
  }
}

void LSSFileMgr::rotateMinutes(const LSSFileLogPtr &file) {
  if (file->fileSize() > 0) {
    char buf[128] = {
        0,
    };
    sprintf(buf, "_%04d-%02d-%02dT%02d%02d", last_year_, last_month_, last_day_,
            last_hour_, last_minute_);
    std::string file_path = file->filePath();
    std::string path = LSSFileUtils::getFilePath(file_path);
    std::string file_name = LSSFileUtils::getFileName(file_path);
    std::string file_ext = LSSFileUtils::getFileExtension(file_path);

    std::ostringstream ss;
    ss << path << file_name << buf << "." << file_ext;
    file->rotate(ss.str());
  }
}

void lssvc::utils::LSSFileMgr::update() {
  bool day_expired{false}; // bool: flags that indicate whether day/hour/minute
                           // is expired or not
  bool hour_expired{false};
  bool minute_expired{false};

  int year, month, day, hour, minute, second; // store the current time
  LSSTime::now(year, month, day, hour, minute, second);
  if (last_day_ == -1) {
    // initialization
    last_year_ = year;
    last_month_ = month;
    last_day_ = day;
    last_hour_ = hour;
    last_minute_ = minute;
  }
  if (last_day_ != day) {
    day_expired = true;
  }
  if (last_hour_ != hour) {
    hour_expired = true;
  }
  if (last_minute_ != minute) {
    minute_expired = true;
  }

  if (!day_expired && !hour_expired && !minute_expired) {
    // currently, we only consider conditions when day/hour/minute is expired.
    // in these cases, log files may be updated.
    return;
  }

  std::lock_guard<std::mutex> lock(lock_);
  for (auto &log : logs_) {
    if (minute_expired && log.second->getRotateType() == kRotateMinute) {
      // rotate mode
      rotateMinutes(log.second);
    }
    if (hour_expired && log.second->getRotateType() == kRotateHour) {
      rotateHours(log.second);
    }
    if (day_expired && log.second->getRotateType() == kRotateDay) {
      rotateDays(log.second);
    }
  }
  last_day_ = day;
  last_hour_ = hour;
  last_year_ = year;
  last_month_ = month;
  last_minute_ = minute;
}
