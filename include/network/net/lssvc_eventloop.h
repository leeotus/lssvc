#ifndef __LSSVC_EVENTLOOP_H__
#define __LSSVC_EVENTLOOP_H__

#include "lssvc_event.h"
#include <string>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

#define LSS_EPOLLEVENTS_MAXSIZE (1024 * 1024)

namespace lssvc::network {

using LSSEventPtr = std::shared_ptr<LSSEvent>;

// @note @todo it's now just a simple server loop class, i haven't introduce
// reactor & thread pool to improve the concurrency of the system
// the focus of this project is the media module, therefore, it's ok to introduce a
// simple but easy-implemented network sever
class LSSEventLoop {
public:
  LSSEventLoop();
  ~LSSEventLoop();

  // @param timeout [in] specifies the maximum wait time (ms) in epoll, -1 == infinite
  void loop(int timeout = -1);

  void quit();

  void addEvent(const LSSEventPtr &ev);
  void delEvent(const LSSEventPtr &ev);
  bool enableWriting(const LSSEventPtr &ev, bool en);
  bool enableReading(const LSSEventPtr &ev, bool en);

private:
  bool running_{false};
  int epoll_fd_{-1};
  std::vector<struct epoll_event> epoll_events_;
  std::unordered_map<int, LSSEventPtr>
      events_; // first: fd, second: pointer to Event object
};

} // namespace lssvc::network

#endif
