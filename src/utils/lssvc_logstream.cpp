#include "utils/lssvc_logstream.h"
#include "utils/lssvc_time.h"
#include <string.h>
#include <thread>

#include "sys/syscall.h"
#include <unistd.h>

using namespace lssvc::utils;

static thread_local pid_t thread_id = 0;

LSSLogger *lssvc::utils::local_logger = nullptr;

const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";
const std::string RESET = "\033[0m"; // reset color

const std::string level_colors[] = {CYAN, GREEN, WHITE, YELLOW, RED};

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
  stream_ << level_colors[level] << log_string[level];
  stream_ << "[" << file_name << ":" << line << "]" << RESET;
  if(func) {
    stream_ << level_colors[level] << "[" << func << "]" << RESET;
  }
}

LSSLogStream::~LSSLogStream() {
  stream_ << "\r\n";
  if(logger_) {
    logger_->write(stream_.str());
  }
}
