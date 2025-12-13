#include "utils/lssvc_task.h"
#include "utils/lssvc_time.h"

namespace lssvc::utils {

LSSTask::LSSTask(const LSSTaskCallback &cb, int64_t interval)
    : interval_(interval), when_(LSSTime::nowMs() + interval_), cb_(cb) {}

LSSTask::LSSTask(const LSSTaskCallback &&cb, int64_t interval)
    : interval_(interval), when_(LSSTime::nowMs() + interval_),
      cb_(std::move(cb)) {}

void LSSTask::run() {
  if(cb_ != nullptr) {
    cb_(shared_from_this());
  }
}

void LSSTask::restart() {
  when_ = interval_ + LSSTime::nowMs();
}

} // namespace lssvc::utils
