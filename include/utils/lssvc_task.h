#ifndef __LSSVC_TASK_H__
#define __LSSVC_TASK_H__

#include <memory>
#include <functional>

namespace lssvc::utils {

class LSSTask; // forward declaration
using LSSTaskPtr = std::shared_ptr<LSSTask>;
using LSSTaskCallback = std::function<void (const LSSTaskPtr &)>;

// @brief a simple Timer task class
// @note [std::shared_ptr] use 'enable_shared_from_this' to avoid using '*this' pointer
class LSSTask : std::enable_shared_from_this<LSSTask> {
public:
  LSSTask(const LSSTaskCallback &cb, int64_t interval); // lvalue version
  LSSTask(const LSSTaskCallback &&cb, int64_t interval); // rvalue version

  void run();
  void restart();

  int64_t when() const {
    return when_;
  }
private:
  int64_t interval_{0}; // ms
  int64_t when_{0};     // when to execute timer callback
  LSSTaskCallback cb_;  // callback
};

}   // namespace lssvc::utils

#endif
