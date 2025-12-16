#include "network/net/lssvc_pipe_event.h"
#include "network/base/lssvc_netlogger.h"
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

using namespace lssvc::network;

LSSPipeEvent::LSSPipeEvent(LSSEventLoop *loop) : LSSEvent(loop) {
  int fd[2] = {0}; // pipe fd
  int ret = ::pipe2(fd, O_NONBLOCK);
  if (ret < 0) {
    NETWORK_ERROR << "failed to open pipe\r\n";
    exit(-1);
  }
  fd_ = fd[0];       // read end
  write_fd_ = fd[1]; // write end
}

LSSPipeEvent::~LSSPipeEvent() {
  if (write_fd_ > 0) {
    ::close(write_fd_);
    write_fd_ = -1;
  }
}

void LSSPipeEvent::onRead() {
  int64_t tmp = 0;
  int ret = ::read(fd_, &tmp, sizeof(tmp));
  if (ret < 0) {
    NETWORK_ERROR << "pipe read error(" << errno << ").\r\n";
    return;
  }
  std::cout << " pipe read tmp:" << tmp << std::endl;
}

void LSSPipeEvent::onClose() {
  if (write_fd_ > 0) {
    ::close(write_fd_);
    write_fd_ = -1;
  }
}

void LSSPipeEvent::onError(const std::string &msg) {
  std::cout << "on error:" << msg << "\r\n";
}

void LSSPipeEvent::write(const char *data, size_t len) {
  ::write(write_fd_, data, len);
}
