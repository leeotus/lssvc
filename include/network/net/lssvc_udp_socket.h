#ifndef __LSSVC_UDP_SOCKET_H__
#define __LSSVC_UDP_SOCKET_H__

#include "lssvc_connection.h"
#include "lssvc_event.h"
#include "lssvc_eventloop.h"
#include "network/base/lssvc_inetaddress.h"
#include "network/base/lssvc_msgbuffer.h"

#include <functional>
#include <list>
#include <memory>
#include <string>

namespace lssvc::network {

class LSSUdpSocket;

using UdpSocketPtr = std::shared_ptr<LSSUdpSocket>;
using UdpSocketMessageCallback =
    std::function<void(const LSSInetAddress &, LSSMsgBuffer &)>;
using UdpSocketWriteCompleteCallback =
    std::function<void(const UdpSocketPtr &)>;
using UdpSocketCloseConnectionCallback =
    std::function<void(const UdpSocketPtr &)>;
using UdpSocketTimeoutCallback = std::function<void(const UdpSocketPtr &)>;

struct UdpTimeoutEntry;

// @brief buffer node, used in the udp packet
struct UdpBufferNode : public BufferNode {
  UdpBufferNode(void *buf, size_t s, struct sockaddr *saddr, socklen_t len)
      : BufferNode(buf, s), sock_addr(saddr), sock_len(len) {}

  struct sockaddr *sock_addr{nullptr}; // target address
  socklen_t sock_len{0};
};

using UdpBufferNodePtr = std::shared_ptr<UdpBufferNode>;

class LSSUdpSocket : public LSSConnection {
  constexpr static int MESSAGE_BUFFER_SIZE = 65535;

public:
  LSSUdpSocket(LSSEventLoop *loop, int sockfd, const LSSInetAddress &localAddr,
               const LSSInetAddress &peerAddr);
  ~LSSUdpSocket();

  template <typename Callback> void setCloseCallback(Callback &&cb) {
    close_cb_ = std::forward<Callback>(cb);
  }

  template <typename Callback> void setRecvMsgCallback(Callback &&cb) {
    message_cb_ = std::forward<Callback>(cb);
  }

  template <typename Callback> void setWriteCompleteCallback(Callback &&cb) {
    write_complete_cb_ = std::forward<Callback>(cb);
  }

  template <typename Callback>
  void setTimeoutCallback(int timeout, Callback &&cb) {
    auto us = std::dynamic_pointer_cast<LSSUdpSocket>(shared_from_this());
    loop_->runAfter(timeout, [this, cb, us]() { cb(us); });
  }

  // @brief handle udp connection timeout events
  void onTimeout();

  /**
   * @brief handle udp connection error events
   * @param msg [in] error messages
   */
  void onError(const std::string &msg) override;

  // @brief handle udp connection read events
  void onRead() override;

  // @brief handle udp connection write events
  void onWrite() override;

  // @brief handle udp connection close events
  void onClose() override;

  void enableCheckIdleTimeout(int32_t max_time);

  // @brief force this udp connection to quit
  void forceClose() override;

  void send(std::list<UdpBufferNodePtr> &list);

  void send(const char *buf, size_t size, struct sockaddr *addr, socklen_t len);

private:
  // @brief extend the life of a udp connection
  void extendLife();

  void sendInLoop(std::list<UdpBufferNodePtr> &list);
  void sendInLoop(const char *buf, size_t size, struct sockaddr *saddr,
                  socklen_t len);

  std::list<UdpBufferNodePtr> buffer_list_;
  bool closed_{false};
  int32_t max_idle_time_{30};
  int32_t message_buffer_size_{MESSAGE_BUFFER_SIZE};

  std::weak_ptr<UdpTimeoutEntry> timeout_entry_;

  LSSMsgBuffer message_buffer_;
  UdpSocketMessageCallback message_cb_;
  UdpSocketWriteCompleteCallback write_complete_cb_;
  UdpSocketCloseConnectionCallback close_cb_;
};

/**
 * @brief insert this timeout entry to the timewheel of the EventLoop
 * once this entry is destructed, the corresponding udp socket will be
 * destroyed too
 * @note using 'extendLife' to increase the ref of this entry
 * @note therefore, the type of the variable timeout_entry in the UdpSocket
 * must be "std::weak_ptr"
 */
struct UdpTimeoutEntry {
  UdpTimeoutEntry(const UdpSocketPtr &c) : conn(c) {}
  ~UdpTimeoutEntry() {
    auto c = conn.lock();
    if (c) {
      c->onTimeout();
    }
  }

  std::weak_ptr<LSSUdpSocket> conn;
};

} // namespace lssvc::network

#endif
