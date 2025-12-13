#include "utils/lssvc_taskmgr.h"
#include "utils/lssvc_time.h"

using namespace lssvc::utils;

void LSSTaskMgr::work() {
  std::lock_guard<std::mutex> lk(lock_);
  int64_t now = LSSTime::nowMs();
  // @todo it may be time-consuming when there are many tasks, use 'epoll' instead
  for(auto it = tasks_.begin(); it != tasks_.end();) {
    if((*it)->when() < now) {
      (*it)->run();
      if((*it)->when() < now) {
        it = tasks_.erase(it);
        continue;
      }
    }
    ++it;
  }
}

bool LSSTaskMgr::add(LSSTaskPtr &task)
{
  std::lock_guard<std::mutex> lk(lock_);
  if(tasks_.find(task) != tasks_.end()) {
    // already in the task queue
    return false;
  }
  tasks_.emplace(task);
  return true;
}

bool lssvc::utils::LSSTaskMgr::del(LSSTaskPtr &task)
{
  std::lock_guard<std::mutex> lk(lock_);
  tasks_.erase(task);
  return true;
}
