#include "utils/lssvc_logger.h"
#include "utils/lssvc_filelog.h"
#include <iostream>

using namespace lssvc::utils;

LSSLogger::LSSLogger(const LSSFileLogPtr &log) : log_(log) {}

void LSSLogger::setLogLevel(const LogLevel &level) {
  level_ = level;
}

LogLevel LSSLogger::getLogLevel() const {
  return level_;
}

void LSSLogger::write(const std::string &msg) {
  if(log_) {
    log_->writeLog(msg);
  } else {
    std::cout << msg;
  }
}
