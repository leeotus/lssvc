#include "utils/lssvc_logstream.h"
#include "utils/lssvc_time.h"
#include <string.h>
#include <thread>

#include "sys/syscall.h"
#include <unistd.h>

using namespace lssvc::utils;

static thread_local pid_t thread_id = 0;
const char *log_string[] = {" TRACE ", " DEBUG ", " INFO ", " WARN ",
                            " ERROR "};

LSSLogStream::LSSLogStream(LSSLogger *logger, const char *file,
                                         int line, LogLevel level,
                                         const char *func)
    : logger_(logger) {
  const char *file_name = strrchr(file, '/'); // not '\\'
  if (file_name != nullptr) {
    file_name = file_name + 1; // get the log file name.
  } else {
    file_name = file;
  }

  stream_ << LSSTime::getISOTime() << " ";
  if (thread_id == 0) {
    thread_id = static_cast<pid_t>(::syscall(SYS_gettid));
  }
  stream_ << thread_id;
  stream_ << log_string[level];
  stream_ << "[" << file_name << ":" << line << "]";
}

LSSLogStream::~LSSLogStream() {
  stream_ << "\r\n";
  logger_->write(stream_.str());
}
