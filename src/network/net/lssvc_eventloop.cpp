#include "network/net/lssvc_eventloop.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_event.h"
#include "utils/lssvc_logger.h"
#include "utils/lssvc_logstream.h"
#include "utils/lssvc_time.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

using namespace lssvc::network;

// @brief use this variable to store EventLoop* object in a thread
// @note the number of 't_local_eventloop' equals to the number of runing
// threads using this 'thread_local' variable, we can judge whether a EventLoop
// object belongs to a thread or not (in order to avoid cross-thread execution)
static thread_local LSSEventLoop *t_local_eventloop = nullptr;

LSSEventLoop::LSSEventLoop()
    : epoll_fd_(-1), epoll_events_(LSS_EPOLLEVENTS_MAXSIZE) {
  epoll_fd_ = ::epoll_create1(0);
  if (epoll_fd_ == -1) {
    NETWORK_ERROR << "Failed to initialize EventLoop!\r\n";
    exit(-1);
  }

  // epoll_events_.reserve(LSS_EPOLLEVENTS_MAXSIZE);
  // epoll_events_.clear();

  if (t_local_eventloop != nullptr) {
    NETWORK_ERROR << "there already had an eventloop.\r\n";
    exit(-1);
  }
  t_local_eventloop = this;
}

LSSEventLoop::~LSSEventLoop() { quit(); }

void LSSEventLoop::quit() { running_ = false; }

void LSSEventLoop::addEvent(const LSSEventPtr &event) {
  if (events_.find(event->getFd()) != events_.end()) {
    // already in the loop
    return;
  }
  event->event_ |= kEventRead;     // friend class
  events_[event->getFd()] = event; // const function

  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = event->event_;
  ev.data.fd = event->getFd();
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event->getFd(), &ev); // add
}

void LSSEventLoop::delEvent(const LSSEventPtr &event) {
  auto it = events_.find(event->getFd());
  if (it == events_.end()) {
    // no such a client fd
    NETWORK_WARN << "Not such a client" << event->getFd() << "\r\n";
    return;
  }
  events_.erase(it);

  // alter it in the epoll
  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = event->event_;
  ev.data.fd = event->getFd();
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event->getFd(), &ev); // delete
}

bool LSSEventLoop::enableReading(const LSSEventPtr &event, bool en) {
  if (events_.find(event->getFd()) == events_.end()) {
    NETWORK_ERROR << "Can't find event fd: " << event->getFd() << "\r\n";
    return false;
  }
  if (en) {
    event->event_ |= kEventRead; // turn on read flag
  } else {
    event->event_ &= ~kEventRead;
  }
  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = event->event_;
  ev.data.fd = event->getFd();
  epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->getFd(), &ev); // mod
  return true;
}

bool LSSEventLoop::enableWriting(const LSSEventPtr &event, bool en) {
  if (events_.find(event->getFd()) == events_.end()) {
    NETWORK_ERROR << "Can't find event fd: " << event->getFd() << "\r\n";
    return false;
  }
  if (en) {
    event->event_ |= kEventWrite; // turn on write flag
  } else {
    event->event_ &= ~kEventWrite;
  }
  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = event->event_;
  ev.data.fd = event->getFd();
  epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->getFd(), &ev); // mod
  return true;
}

void LSSEventLoop::loop(int timeout) {
  running_ = true; // set running_ flag true
  while (running_) {
    memset(&epoll_events_[0], 0,
           sizeof(struct epoll_event) * epoll_events_.size());
    int nready =
        ::epoll_wait(epoll_fd_, (struct epoll_event *)&epoll_events_[0],
                     static_cast<int>(epoll_events_.size()), timeout);
    if (nready >= 0) {
      for (size_t i = 0; i < nready; ++i) {
        struct epoll_event &ev = epoll_events_[i];
        if (ev.data.fd <= 0) {
          continue;
        }
        if (events_.find(ev.data.fd) == events_.end()) {
          continue;
        }

        LSSEventPtr &event = events_[ev.data.fd];
        if (ev.events & EPOLLERR) {
          // meets a error
          int error = 0;
          socklen_t len = sizeof(error);
          getsockopt(event->getFd(), SOL_SOCKET, SO_ERROR, &error, &len);

          event->onError(strerror(error));
        } else if ((ev.events & EPOLLHUP) && !(ev.events & EPOLLIN)) {
          event->onClose();
        } else if (ev.events & (EPOLLIN | EPOLLPRI)) {
          event->onRead();
        } else if (ev.events & EPOLLOUT) {
          event->onWrite();
        }
      }

      if (nready >= epoll_events_.size() - LSS_EPOLLEVENTS_RESIZE_DELTA) {
        epoll_events_.resize(epoll_events_.size() * LSS_EPOLLEVENTS_GROWFACTOR);
      }

      runTask();

      int64_t now = lssvc::utils::LSSTime::nowMs();
      wheel_.onTimer(now);

    } else if (nready < 0) {
      NETWORK_ERROR << "epoll wait meets an error: " << errno << "\r\n";
    }
  }
}

void LSSEventLoop::enqueueTask(const std::function<void()> &f) {
  if (isInLoopThread()) {
    f();
  } else {
    std::lock_guard<std::mutex> lock(lock_);
    tasks_.push(f);

    wakeUp();
  }
}

void LSSEventLoop::enqueueTask(std::function<void()> &&f) {
  if (isInLoopThread()) {
    f();
  } else {
    std::lock_guard<std::mutex> lock(lock_);
    tasks_.push(std::move(f));

    wakeUp();
  }
}

void LSSEventLoop::checkInLoopThread() {
  if (!isInLoopThread()) {
    NETWORK_ERROR << "It is forbidden to run loop on other thread.\r\n";
    exit(-1);
  }
}

bool LSSEventLoop::isInLoopThread() const { return this == t_local_eventloop; }

void LSSEventLoop::initPipe() {
  if (pipe_ == nullptr) {
    pipe_ = std::make_shared<LSSPipeEvent>(this);
    this->addEvent(pipe_);
  }
}

void LSSEventLoop::wakeUp() {
  initPipe();
  int64_t tmp = 1;
  pipe_->write((const char *)&tmp, sizeof(tmp));
}

void LSSEventLoop::runTask() {
  std::lock_guard<std::mutex> lock(lock_);
  while (!tasks_.empty()) {
    auto &f = tasks_.front();
    f();
    tasks_.pop();
  }
}

void LSSEventLoop::insertEntry(uint32_t delay, EntryPtr entry) {
  enqueueTask([this, delay, entry]() { wheel_.insertEntry(delay, entry); });
}

void LSSEventLoop::runAfter(int delay, const std::function<void()> &cb) {
  enqueueTask([this, delay, cb]() { wheel_.runAfter(delay, cb); });
}

void LSSEventLoop::runAfter(int delay, std::function<void()> &&cb) {
  enqueueTask([this, delay, cb]() { wheel_.runAfter(delay, cb); });
}

void LSSEventLoop::runEvery(int interval, const std::function<void()> &cb) {
  enqueueTask([this, interval, cb]() { wheel_.runEvery(interval, cb); });
}

void LSSEventLoop::runEvery(int interval, std::function<void()> &&cb) {
  enqueueTask([this, interval, cb]() { wheel_.runEvery(interval, cb); });
}
