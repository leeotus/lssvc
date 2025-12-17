#ifndef __LSSVC_EVENTLOOP_H__
#define __LSSVC_EVENTLOOP_H__

#include "lssvc_event.h"
#include "lssvc_pipe_event.h"

#include <string>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

#include <mutex>
#include <functional>
#include <queue>

#define LSS_EPOLLEVENTS_MAXSIZE (1024 * 512)
#define LSS_EPOLLEVENTS_GROWFACTOR (2)
#define LSS_EPOLLEVENTS_RESIZE_DELTA (128)

namespace lssvc::network {

using LSSEventPtr = std::shared_ptr<LSSEvent>;

/**
 * @brief EventLoop utilizes muduo's work, it ensures the "lock-free efficiency" of
 * single EventLoop single-thread execution, but also can utilize muti-core CPUs to
 * improve overall processing capabilities
 */
class LSSEventLoop {
public:
  LSSEventLoop();
  ~LSSEventLoop();

  // @param timeout [in] specifies the maximum wait time (ms) in epoll, -1 == infinite
  void loop(int timeout = -1);

  // @brief set running_ flag false to end this eventloop
  void quit();

  /**
   * @brief add a new Event object to the current EventLoop
   * @param ev [in] pointer to the Event object
   * @note if the input Event object has already in EventLoop, do nothing
   */
  void addEvent(const LSSEventPtr &ev);

  /**
   * @brief delete the Event object, which the input 'ev' points to in the EventLoop
   * @param ev [in] pointer to the Event object
   */
  void delEvent(const LSSEventPtr &ev);

  /**
   * @brief set epoll event flags (enable EPOLLOUT or not)
   * @param ev [in] pointer to the Event object (which stores the epoll event flag)
   * @param en [in] true/false
   * @return true if success, else false
   */
  bool enableWriting(const LSSEventPtr &ev, bool en);

  // @see same as 'enableWriting', but set EPOLLIN flag instead
  bool enableReading(const LSSEventPtr &ev, bool en);

  void checkInLoopThread();

  bool isInLoopThread() const;

  /**
   * @brief enqueue a task
   * @param f function representing a task
   * @note we use round-robin load balancing algo.(@see EventLoopThreadPool) to assign the input task into
   * EventLoops (one EventLoop per thread), therefore, this task may be assigned back(@see 'getNextLoop' in
   * EventLoopThreadPool) to the current running thread(/EventLoop). In this case, just run it without
   * enqueue it into the task queue
   */
  void enqueueTask(const std::function<void()> &f);   // lvalue version
  void enqueueTask(const std::function<void()> &&f);  // rvalue version

private:

  // @brief initialize pipe_, injecting the read end of this pipe into epoll
  void initPipe();

  /**
   * @brief wake up loop
   * @note runing loop will be blocked due to the function 'epoll_wait', you need to
   * use this 'wakeUp' function to 'wake up' the blocked loop in order to add/mod/... a new client connection
   */
  void wakeUp();

  // @brief universal callbacks for non-IO events, like callback of scheduled tasks etc.
  void runTask();

  bool running_{false};
  int epoll_fd_{-1};
  std::vector<struct epoll_event> epoll_events_;
  std::unordered_map<int, LSSEventPtr>
      events_; // first: fd, second: pointer to Event object

  std::queue<std::function<void()>> tasks_;   // task queue
  std::mutex lock_;  // for task queue
  LSSPipeEventPtr pipe_;
};

} // namespace lssvc::network

#endif
