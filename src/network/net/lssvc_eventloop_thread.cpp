#include "network/net/lssvc_eventloop_thread.h"
#include "network/net/lssvc_eventloop.h"

using namespace lssvc::network;

LSSEventLoopThread::LSSEventLoopThread() : thread_([this]() { start(); }) {}

LSSEventLoopThread::~LSSEventLoopThread() {
  run();
  if (loop_ != nullptr) {
    loop_->quit();
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

void LSSEventLoopThread::run() {
  std::call_once(once_, [this]() {
    // do this lambda function only once
    {
      std::lock_guard<std::mutex> lock(lock_);
      running_ = true; // set running_ true
      cond_.notify_all();
    }
    auto f = promise_loop_.get_future(); // block
    f.get();
  });
}

LSSEventLoop *LSSEventLoopThread::loop() const { return loop_; }

void LSSEventLoopThread::start() {
  LSSEventLoop loop; // create an EventLoop object to handle epoll events

  std::unique_lock<std::mutex> lock(lock_);
  cond_.wait(lock, [this]() { return running_; });

  // ensure consistency of the life cycle between the EventLoop
  // and the corresponding EventLoopThread
  loop_ = &loop;
  promise_loop_.set_value(1);
  loop.loop();
  loop_ = nullptr;
}
