#include "network/net/lssvc_udp_socket.h"
#include "network/base/lssvc_netlogger.h"
#include <string>

using namespace lssvc::network;

LSSUdpSocket::LSSUdpSocket(LSSEventLoop *loop, int sockfd,
                           const LSSInetAddress &localAddr,
                           const LSSInetAddress &peerAddr)
    : LSSConnection(loop, sockfd, localAddr, peerAddr),
      message_buffer_(message_buffer_size_) {}

LSSUdpSocket::~LSSUdpSocket() {}

void LSSUdpSocket::onTimeout() {
  NETWORK_TRACE << "host:" << getPeerAddr().toIpWithPort() << " timeout.";
  // close it
  onClose();
}

void LSSUdpSocket::onError(const std::string &msg) {
  // meets an error
  NETWORK_ERROR << "host:" << getPeerAddr().toIpWithPort() << " error:" << msg;
  onClose();
}

void LSSUdpSocket::onRead() {
  if (closed_) {
    // already closed
    NETWORK_WARN << "host: " << getPeerAddr().toIpWithPort() << " had closed.";
    return;
  }
  // extend the life of this udp connection, for it's still working now.
  extendLife();
  while (true) {
    struct sockaddr_in6 sock_addr;
    socklen_t len = sizeof(struct sockaddr_in6);
    auto ret =
        ::recvfrom(fd_, message_buffer_.beginWrite(), message_buffer_size_, 0,
                   (struct sockaddr *)&sock_addr, &len);
    if (ret > 0) {
      LSSInetAddress peeraddr;
      // record the written data's size
      message_buffer_.hasWritten(ret);

      // record the address and the port of the incoming client
      if (sock_addr.sin6_family == AF_INET) {
        // ipv4
        char ip[16] = {
            0,
        };
        struct sockaddr_in *saddr = (struct sockaddr_in *)&sock_addr;
        ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
        peeraddr.setAddr(ip);
        peeraddr.setPort(ntohs(saddr->sin_port));
        peeraddr.setIsIpv6(false);
      } else if (sock_addr.sin6_family == AF_INET6) {
        // ipv6
        char ip[INET6_ADDRSTRLEN] = {
            0,
        };
        ::inet_ntop(AF_INET6, &(sock_addr.sin6_addr), ip, sizeof(ip));
        peeraddr.setAddr(ip);
        peeraddr.setPort(ntohs(sock_addr.sin6_port));
        peeraddr.setIsIpv6(true);
      }

      if (message_cb_) {
        // if set message_cb
        message_cb_(peeraddr, message_buffer_);
      }
      // remove all date in the buffer
      message_buffer_.retrieveAll();
    } else if (ret < 0) {
      if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
        NETWORK_ERROR << "host: " << getPeerAddr().toIpWithPort()
                      << " error occurs:" << errno;
        onClose();
        return;
      }
      break;
    }
  }
}

void LSSUdpSocket::onWrite() {
  if (closed_) {
    NETWORK_WARN << "host: " << getPeerAddr().toIpWithPort() << " had closed.";
    return;
  }
  // extend the life of this udp connection
  extendLife();
  while (true) {
    if (!buffer_list_.empty()) {
      // write data
      auto buf = buffer_list_.front(); // take out 1 buffer
      int ret =
          ::sendto(fd_, buf->addr, buf->size, 0, buf->sock_addr, buf->sock_len);
      if (ret > 0) {
        // ok
        buffer_list_.pop_front();
      } else if (ret < 0) {
        if (errno != EWOULDBLOCK && errno != EINTR && errno != EAGAIN) {
          // error occurs
          NETWORK_ERROR << "host: " << getPeerAddr().toIpWithPort()
                        << " error: " << errno;
          // close this connection
          onClose();
          return;
        }
        break;
      }
    }
    if (buffer_list_.empty()) {
      // already done
      if (write_complete_cb_) {
        // if set
        write_complete_cb_(
            std::dynamic_pointer_cast<LSSUdpSocket>(shared_from_this()));
      }
      break;
    }
  }
}

void LSSUdpSocket::onClose() {
  if (!closed_) {
    closed_ = true;
    if (close_cb_) {
      close_cb_(std::dynamic_pointer_cast<LSSUdpSocket>(shared_from_this()));
    }
    // use Event::close to close this socket fd
    LSSEvent::close();
  }
}

void LSSUdpSocket::forceClose() {
  // close this udp connection in the loop
  loop_->enqueueTask([this]() { onClose(); });
}

void LSSUdpSocket::enableCheckIdleTimeout(int32_t max_time) {
  auto tp = std::make_shared<UdpTimeoutEntry>(
      std::dynamic_pointer_cast<LSSUdpSocket>(shared_from_this()));
  max_idle_time_ = max_time;
  timeout_entry_ = tp;
  loop_->insertEntry(max_time, tp);
}

void LSSUdpSocket::extendLife() {
  auto tp = timeout_entry_.lock();
  if (tp) {
    loop_->insertEntry(max_idle_time_, tp);
  }
}

void LSSUdpSocket::send(std::list<UdpBufferNodePtr> &list) {
  loop_->enqueueTask([this, &list]() { sendInLoop(list); });
}

void LSSUdpSocket::send(const char *buf, size_t size, struct sockaddr *addr,
                        socklen_t len) {
  loop_->enqueueTask(
      [this, buf, size, addr, len] { sendInLoop(buf, size, addr, len); });
}

void LSSUdpSocket::sendInLoop(std::list<UdpBufferNodePtr> &list) {
  for (auto &i : list) {
    buffer_list_.emplace_back(i);
  }
  if (!buffer_list_.empty()) {
    enableWriting(true);
  }
}
void LSSUdpSocket::sendInLoop(const char *buf, size_t size,
                              struct sockaddr *saddr, socklen_t len) {
  if (buffer_list_.empty()) {
    // if buffer_list is empty, send this single buffer
    auto ret = ::sendto(fd_, buf, size, 0, saddr, len);
    if (ret > 0) {
      return;
    }
  }
  // else package this buffer into a UdpBufferNode and insert into the
  // buffer_list
  auto node = std::make_shared<UdpBufferNode>((void *)buf, size, saddr, len);
  buffer_list_.emplace_back(node);
  enableWriting(true);
}
