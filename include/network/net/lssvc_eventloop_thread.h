#ifndef __LSSVC_EVENTLOOP_THREAD_H__
#define __LSSVC_EVENTLOOP_THREAD_H__

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include "utils/noncopyable.h"

namespace lssvc {

namespace network {

class LSSEventLoop;

// @brief thread class used for Threadpool
class LSSEventLoopThread : public utils::NonCopyable {
public:
  LSSEventLoopThread();
  ~LSSEventLoopThread();

  void run();

  // getter
  LSSEventLoop *loop() const;
  std::thread &getThread();

private:
  /**
   * @brief a loop per thread
   * @note this function, which used in the lambda function (in the default
   * constructor), is the entry of 'EventLoopThread', it waits util the flag
   * 'running_' is true, and then make the 'EventLoop' work properly.
   */
  void start();

  LSSEventLoop *loop_{nullptr};
  std::thread thread_;
  bool running_{false};
  std::mutex lock_;
  std::condition_variable cond_;
  std::once_flag once_;
  std::promise<int> promise_loop_;
};

} // namespace network

} // namespace lssvc

#endif
