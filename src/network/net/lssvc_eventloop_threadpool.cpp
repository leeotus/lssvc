#include "network/net/lssvc_eventloop_threadpool.h"
#include <pthread.h>

namespace lssvc::network {

// bind the input thread to the cpus
// reduce the overhead of thread switching
// @note this function now can only used in Linux system
inline void bind_cpu(std::thread &t, int n) {
  cpu_set_t cpu;
  CPU_ZERO(&cpu);
  CPU_SET(n, &cpu);

  pthread_setaffinity_np(t.native_handle(), sizeof(cpu), &cpu);
}
} // namespace lssvc::network

using namespace lssvc::network;

LSSEventLoopThreadPool::LSSEventLoopThreadPool(int thread_num, int start,
                                               int cpus) {
  if (thread_num <= 0) {
    thread_num = 1;
  }
  for (int i = 0; i < thread_num; ++i) {
    threads_.emplace_back(std::make_shared<LSSEventLoopThread>());
    if (cpus > 0) {
      int n = (start + i) % cpus;
      bind_cpu(threads_.back()->getThread(), n);
    }
  }
}

LSSEventLoopThreadPool::~LSSEventLoopThreadPool() {}

size_t LSSEventLoopThreadPool::size() { return threads_.size(); }

std::vector<LSSEventLoop *> LSSEventLoopThreadPool::getLoops() const {
  std::vector<LSSEventLoop *> ret{};
  for(auto &t : threads_) {
    ret.emplace_back(t->loop());
  }
  return ret;
}

LSSEventLoop *LSSEventLoopThreadPool::getNextLoop() {
  int index = loop_index_;
  loop_index_ += 1;
  return threads_[index % threads_.size()]->loop();
}

void LSSEventLoopThreadPool::start() {
  for(auto &t : threads_) {
    t->run();
  }
}
