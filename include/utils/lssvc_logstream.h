#ifndef __LSSVC_LOG_STREAM_H__
#define __LSSVC_LOG_STREAM_H__

#include "lssvc_singleton.h"
#include "lssvc_logger.h"
#include "lssvc_singleton.h"
#include <sstream>
#include <iostream>

#define LSSVC_LOG_ENABLE 0


namespace lssvc::utils {

extern LSSLogger *local_logger;

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
#define LSSVC_LOG_TRACE                                                        \
  if (lssvc::utils::local_logger &&                                            \
      lssvc::utils::local_logger->getLogLevel() <= lssvc::utils::kTrace)       \
  lssvc::utils::LSSLogStream(lssvc::utils::local_logger, __FILE__, __LINE__,   \
                             lssvc::utils::kTrace, __func__)
#define LSSVC_LOG_DEBUG                                                        \
  if (lssvc::utils::local_logger &&                                            \
      lssvc::utils::local_logger->getLogLevel() <= lssvc::utils::kDebug)       \
  lssvc::utils::LSSLogStream(lssvc::utils::local_logger, __FILE__, __LINE__,   \
                             lssvc::utils::kDebug, __func__)
#define LSSVC_LOG_INFO                                                         \
  if (lssvc::utils::local_logger &&                                            \
      lssvc::utils::local_logger->getLogLevel() <= lssvc::utils::kInfo)        \
  lssvc::utils::LSSLogStream(lssvc::utils::local_logger, __FILE__, __LINE__,   \
                             lssvc::utils::kInfo)
#define LSSVC_LOG_WARN                                                         \
  if (lssvc::utils::local_logger)                                              \
  lssvc::utils::LSSLogStream(lssvc::utils::local_logger, __FILE__, __LINE__,   \
                             lssvc::utils::kWarn)
#define LSSVC_LOG_ERROR                                                        \
  if (lssvc::utils::local_logger)                                              \
  lssvc::utils::LSSLogStream(lssvc::utils::local_logger, __FILE__, __LINE__,   \
                             lssvc::utils::kError)
#endif

#endif
