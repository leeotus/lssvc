#ifndef __LSSVC_LOGGER_H__
#define __LSSVC_LOGGER_H__

#include "lssvc_filelog.h"
#include "noncopyable.h"
#include <string>

#define g_lsslogger lssvc::utils::LSSSingleton<lssvc::utils::LSSLogger>::getInstance()

namespace lssvc::utils{

enum LogLevel {
  kTrace,
  kDebug,
  kInfo,
  kWarn,
  kError,
  kMaxNumOfLogLevel   // for(int i=kTrace, i < kMaxNumOfLogLevel; ++i)
};

// currently we just print messages in the terminal(for log file, see 'lssvc_filelog.h')
class LSSLogger : public NonCopyable {
public:
  LSSLogger() = default;
  LSSLogger(const LSSFileLogPtr &log);
  ~LSSLogger() = default;

  void setLogLevel(const LogLevel &level);
  LogLevel getLogLevel() const;
  void write(const std::string &msg);
private:
  LogLevel level_{kDebug};
  LSSFileLogPtr log_;
};

}  // lssvc::utils

#endif
