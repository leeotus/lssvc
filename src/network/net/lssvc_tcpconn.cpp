#include "network/net/lssvc_tcpconn.h"
#include "network/base/lssvc_msgbuffer.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_event.h"

#include <list>
#include <sys/socket.h>
#include <unistd.h>

using namespace lssvc::network;

LSSTcpConnection::LSSTcpConnection(LSSEventLoop *loop, int sockfd,
                                   const LSSInetAddress &localAddr,
                                   const LSSInetAddress &peerAddr)
    : LSSConnection(loop, sockfd, localAddr, peerAddr) {}

LSSTcpConnection::~LSSTcpConnection() { onClose(); }

void LSSTcpConnection::onRead() {
  if (closed_.load()) {
    // this socket has already close
    NETWORK_TRACE << "host: " << getPeerAddr().toIpWithPort()
                  << "had close\r\n";
    return;
  }
  while (true) {
    int err;
    auto ret = message_buffer_.readFd(fd_, &err);
    if (ret > 0) {
      if (message_cb_) {
        message_cb_(
            std::dynamic_pointer_cast<LSSTcpConnection>(shared_from_this()),
            message_buffer_);
      }
    } else if (ret == 0) {
      onClose();
      break;
    } else {
      if (err != EAGAIN && err != EINTR && err != EWOULDBLOCK) {
        NETWORK_ERROR << "read error:" << err << "\r\n";
        onClose();
      }
      break;
    }
  }
}

void LSSTcpConnection::onClose() {
  loop_->checkInLoopThread();
  if (!closed_.load()) {
    closed_.store(true);
    if (close_cb_) {
      close_cb_(
          std::dynamic_pointer_cast<LSSTcpConnection>(shared_from_this()));
    }

    // close this tcp connection
    LSSEvent::close();
  }
}

void LSSTcpConnection::onError(const std::string &errmsg) {
  NETWORK_ERROR << "host: " << getPeerAddr().toIpWithPort()
                << " , error:" << errmsg << "\r\n";
  onClose();
}

void LSSTcpConnection::forceClose() {
  loop_->enqueueTask([this]() { onClose(); });
}

void LSSTcpConnection::onWrite() {
  if (closed_.load()) {
    // already close
    NETWORK_ERROR << "host: " << getPeerAddr().toIpWithPort()
                  << " had closed.\r\n";
    return;
  }
  if (!io_vec_list_.empty()) {
    while (true) {
      auto ret = ::writev(fd_, &io_vec_list_[0], io_vec_list_.size());
      if (ret >= 0) {
        while (ret > 0) {
          if (io_vec_list_.front().iov_len > ret) {
            io_vec_list_.front().iov_base += ret;
            io_vec_list_.front().iov_len -= ret;
            break;
          } else {
            ret -= io_vec_list_.front().iov_len;
            io_vec_list_.erase(io_vec_list_.begin());
          }
        }
        if (io_vec_list_.empty()) {
          enableWriting(false);
          if (write_complete_cb_) {
            write_complete_cb_(std::dynamic_pointer_cast<LSSTcpConnection>(
                shared_from_this()));
          }
          return;
        }
      } else if (ret < 0) {
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
          NETWORK_ERROR << "host: " << getPeerAddr().toIpWithPort()
                        << " write error:" << errno;
          onClose();
          return;
        }
        break;
      }
    }
  } else {
    enableWriting(false);
    if (write_complete_cb_) {
      write_complete_cb_(
          std::dynamic_pointer_cast<LSSTcpConnection>(shared_from_this()));
    }
  }
}

void LSSTcpConnection::setTimeoutCallback(int timeout,
                                          const TimeoutCallback &cb) {
  auto ptr = std::dynamic_pointer_cast<LSSTcpConnection>(shared_from_this());
  loop_->runAfter(timeout, [&cb, &ptr]() { cb(ptr); });
}

void LSSTcpConnection::setTimeoutCallback(int timeout, TimeoutCallback &&cb) {
  auto ptr = std::dynamic_pointer_cast<LSSTcpConnection>(shared_from_this());
  loop_->runAfter(timeout, [cb, &ptr]() { cb(ptr); });
}

void LSSTcpConnection::onTimeout() {
  // prevent TCP connections from occupying resources
  NETWORK_TRACE << "host: " << getPeerAddr().toIpWithPort()
                << " timeout and close it.\r\n";
  onClose();
}

void LSSTcpConnection::enableCheckIdleTimeout(int32_t max_time) {
  // let the weak pointer hold "this"
  auto tp = std::make_shared<TimeoutEntry>(
      std::dynamic_pointer_cast<LSSTcpConnection>(shared_from_this()));
  max_idle_time_ = max_time;
  timeout_entry_ = tp;
  loop_->insertEntry(max_time, tp);
}

void LSSTcpConnection::extendLife() {
  auto tp = timeout_entry_.lock();
  if (tp) {
    // extend connection life by adding reference of a shared_ptr object
    loop_->insertEntry(max_idle_time_, tp);
  }
}

void LSSTcpConnection::send(std::list<BufferNodePtr> &list) {
  loop_->enqueueTask([this, &list]() { sendInLoop(list); });
}

void LSSTcpConnection::send(const char *buf, size_t size) {
  loop_->enqueueTask([this, buf, size]() { sendInLoop(buf, size); });
}

void LSSTcpConnection::sendInLoop(std::list<BufferNodePtr> &list) {
  if (closed_.load()) {
    // already close
    NETWORK_ERROR << "host: " << getPeerAddr().toIpWithPort()
                  << " had closed.\r\n";
    return;
  }
  for (auto &l : list) {
    struct iovec vec;
    vec.iov_base = (void *)l->addr;
    vec.iov_len = l->size;

    io_vec_list_.push_back(vec);
  }
  if (!io_vec_list_.empty()) {
    enableWriting(true);
  }
}

void LSSTcpConnection::sendInLoop(const char *buf, size_t size) {
  if (closed_.load()) {
    // already close
    NETWORK_ERROR << "host: " << getPeerAddr().toIpWithPort()
                  << " had closed.\r\n";
    return;
  }
  size_t send_len = 0;
  if (io_vec_list_.empty()) {
    send_len = ::write(fd_, buf, size);
    if (send_len < 0) {
      if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
        NETWORK_ERROR << "host: " << getPeerAddr().toIpWithPort()
                      << " write error:" << errno;
        onClose();
        return;
      }
      send_len = 0;
    }
    size -= send_len;
    if (size == 0) {
      if (write_complete_cb_) {
        write_complete_cb_(
            std::dynamic_pointer_cast<LSSTcpConnection>(shared_from_this()));
      }
      return;
    }
  }
  if (size > 0) {
    struct iovec vec;
    vec.iov_base = (void *)buf + send_len;
    vec.iov_len = size;

    io_vec_list_.push_back(vec);
    enableWriting(true);
  }
}
