#include "network/net/lssvc_event.h"
#include "network/net/lssvc_eventloop.h"

#include <unistd.h>

using namespace lssvc::network;

LSSEvent::LSSEvent() : loop_(nullptr), fd_(-1), event_(0) {}

LSSEvent::LSSEvent(LSSEventLoop *loop) : loop_(loop) {}

LSSEvent::LSSEvent(LSSEventLoop *loop, int fd) : loop_(loop), fd_(fd), event_(0) {}

LSSEvent::~LSSEvent() {
  if (fd_ > 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

// @note the reason why we need to hold a member variable of EventLoop(i.e,
// loop_) is because we can't alter a Event's flag (like from kEventRead to
// kEventWrite) by simply reassigning 'event_', 'epoll_ctl' should be called
// necessarily in the EventLoop object

bool LSSEvent::enableReading(bool en) {
  return loop_->enableReading(shared_from_this(), en);
}

bool LSSEvent::enableWriting(bool en) {
  return loop_->enableWriting(shared_from_this(), en);
}

int LSSEvent::getFd() const { return fd_; }
