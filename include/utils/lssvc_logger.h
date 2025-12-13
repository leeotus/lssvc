#ifndef __LSSVC_LOGGER_H__
#define __LSSVC_LOGGER_H__

#include "noncopyable.h"
#include <string>

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
  LSSLogger();
  ~LSSLogger() = default;

  void setLogLevel(const LogLevel &level);
  LogLevel getLogLevel() const;
  void write(const std::string &msg);
private:
  LogLevel level_{kDebug};
};

}  // lssvc::utils

#endif
