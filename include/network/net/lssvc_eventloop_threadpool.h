#ifndef __LSSVC_EVENTLOOP_THREADPOOL_H__
#define __LSSVC_EVENTLOOP_THREADPOOL_H__

#include "utils/noncopyable.h"
#include "lssvc_eventloop_thread.h"
#include "lssvc_eventloop.h"

#include <memory>
#include <vector>
#include <atomic>

namespace lssvc {
namespace network {

using LSSEventLoopThreadPtr = std::shared_ptr<LSSEventLoopThread>;

class LSSEventLoopThreadPool : public utils::NonCopyable {
public:
  LSSEventLoopThreadPool(int thread_num, int start=0, int cpus=4);
  ~LSSEventLoopThreadPool();

  std::vector<LSSEventLoop *> getLoops() const;

  // @note round-robin load balancing
  LSSEventLoop *getNextLoop();

  size_t size(); // number of total threads

  void start();

private:
  std::vector<LSSEventLoopThreadPtr> threads_;
  std::atomic_int32_t loop_index_{0};
};

} // network
} // lssvc

#endif
