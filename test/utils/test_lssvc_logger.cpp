#include <iostream>
#include "utils/lssvc_logstream.h"
#include "utils/lssvc_logger.h"
#include "utils/lssvc_singleton.h"

using namespace lssvc::utils;

int main(int argc, char **argv) {
  local_logger = new LSSLogger();
  local_logger->setLogLevel(kTrace);
  LSSVC_LOG_DEBUG << "hello world\r\n";
  LSSVC_LOG_INFO << "hello world\r\n";
  LSSVC_LOG_TRACE << "hello world\r\n";
  LSSVC_LOG_WARN << "hello world\r\n";
  LSSVC_LOG_ERROR << "hello world\r\n";
  delete local_logger;
  return 0;
}
