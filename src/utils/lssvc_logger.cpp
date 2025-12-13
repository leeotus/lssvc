#include "utils/lssvc_logger.h"
#include <iostream>

using namespace lssvc::utils;

LSSLogger::LSSLogger() {}

void LSSLogger::setLogLevel(const LogLevel &level) {
  level_ = level;
}

LogLevel LSSLogger::getLogLevel() const {
  return level_;
}

void LSSLogger::write(const std::string &msg) {
  std::cout << msg;
}
