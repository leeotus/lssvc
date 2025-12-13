#ifndef __LSSVC_TASK_MANAGER_H__
#define __LSSVC_TASK_MANAGER_H__

#include "lssvc_singleton.h"
#include "lssvc_task.h"
#include "noncopyable.h"
#include <mutex>
#include <unordered_set>

#define gLSSTaskMgr lssvc::utils::LSSSingleton<lssvc::utils::LSSTaskMgr>::getInstance()

namespace lssvc::utils {

// @brief manage (to run/add/delete...) tasks in the task queue, avoid using 'LSSTask' directly
class LSSTaskMgr : public NonCopyable {
public:
  LSSTaskMgr() = default;
  ~LSSTaskMgr() = default;

  void work();

  bool add(LSSTaskPtr &task);

  bool del(LSSTaskPtr &task);

private:
  // @todo maybe use #include <atomic> ?

  std::mutex lock_;   // mutex for task queue

  // @note currenty, we use 'unordered_set' to store tasks. In this way,
  // the tasks are not sorted by 'interval' or 'when'
  std::unordered_set<LSSTaskPtr> tasks_;
};

} // namespace lssvc::utils


#endif
