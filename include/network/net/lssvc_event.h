#ifndef __LSSVC_EVENT_H__
#define __LSSVC_EVENT_H__

#include <memory>
#include <string>
#include <sys/epoll.h>

namespace lssvc::network {

// @note currently, we only support epoll. (others (select and poll) are not
// considered)
// @todo io_uring may be implemented in the future

class LSSEventLoop;
constexpr int kEventRead = (EPOLLIN | EPOLLPRI | EPOLLET); // use ET
constexpr int kEventWrite = (EPOLLOUT | EPOLLET);

class LSSEvent : public std::enable_shared_from_this<LSSEvent> {
  friend class LSSEventLoop;

public:
  LSSEvent();

  LSSEvent(LSSEventLoop *loop);

  /**
   * @brief when the server accept a client, we get the fd of a client in this
   * 'EventLoop', then package this client(with its fd) into a Event object
   */
  LSSEvent(LSSEventLoop *loop, int fd);

  ~LSSEvent();

  // @brief handle read events
  virtual void onRead() {}

  // @brief handle write events
  virtual void onWrite() {}

  // @brief handle close events
  virtual void onClose() {}

  // @brief handle error events
  virtual void onError(const std::string &msg) {}

  bool enableWriting(bool en);
  bool enableReading(bool en);

  // @brief get the fd value
  int getFd() const;

  // @brief close this event object/socket
  void close();

protected:
  LSSEventLoop
      *loop_; // the corresponding loop(server) holding this event(client)
  int fd_;    // file description for epoll
  int event_;
};

} // namespace lssvc::network

#endif
