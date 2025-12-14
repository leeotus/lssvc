#ifndef __LSSVC_LOG_STREAM_H__
#define __LSSVC_LOG_STREAM_H__

#include "lssvc_logger.h"
#include <sstream>
#include <iostream>

#define LSSVC_LOG_ENABLE 1

#if LSSVC_LOG_ENABLE
#define LSSVC_LOG_TRACE                                                        \
  if (g_lsslogger->getLogLevel() <= lssvc::utils::kTrace)                      \
  lssvc::utils::LSSLogStream(g_lsslogger, __FILE__, __LINE__,                  \
                             lssvc::utils::kTrace, __func__)

#define LSSVC_LOG_DEBUG                                                        \
  if (g_lsslogger->getLogLevel() <= lssvc::utils::kDebug)                      \
  lssvc::utils::LSSLogStream(g_lsslogger, __FILE__, __LINE__,                  \
                             lssvc::utils::kDebug, __func__)

#define LSSVC_LOG_INFO                                                         \
  if (g_lsslogger->getLogLevel() <= lssvc::utils::kInfo)                       \
  lssvc::utils::LSSLogStream(g_lsslogger, __FILE__, __LINE__,                  \
                             lssvc::utils::kInfo)

#define LSSVC_LOG_WARN                                                         \
  lssvc::utils::LSSLogStream(g_lsslogger, __FILE__, __LINE__,                  \
                             lssvc::utils::kWarn)

#define LSSVC_LOG_ERROR                                                        \
  lssvc::utils::LSSLogStream(g_lsslogger, __FILE__, __LINE__,                  \
                             lssvc::utils::kError)
#else
// use std::cout instead
#define LSSVC_LOG_TRACE std::cout
#define LSSVC_LOG_DEBUG std::cout
#define LSSVC_LOG_INFO std::cout
#define LSSVC_LOG_WARN std::cout
#define LSSVC_LOG_ERROR std::cout
#endif

namespace lssvc::utils {

class LSSLogStream {
public:
  LSSLogStream(LSSLogger *logger, const char *file, int line, LogLevel level,
               const char *func = nullptr);
  ~LSSLogStream();

  template <typename T> LSSLogStream &operator<<(const T &value) {
    stream_ << value;
    return *this; // chain call
  }

private:
  std::ostringstream stream_;
  LSSLogger *logger_{nullptr};
};
} // namespace lssvc::utils

#endif
