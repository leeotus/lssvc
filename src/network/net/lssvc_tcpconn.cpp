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
                                   LSSInetAddress &peerAddr)
    : LSSConnection(loop, sockfd, localAddr, peerAddr) {}

LSSTcpConnection::~LSSTcpConnection() { onClose(); }

void LSSTcpConnection::onRead() {
  if (closed_.load()) {
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
  }
  if (size > 0) {
    struct iovec vec;
    vec.iov_base = (void *)buf + send_len;
    vec.iov_len = size;

    io_vec_list_.push_back(vec);
    enableWriting(true);
  }
}
