#ifndef __LSSVC_TCP_CONNECTION_H__
#define __LSSVC_TCP_CONNECTION_H__

#include "lssvc_connection.h"
#include "network/base/lssvc_inetaddress.h"
#include "network/base/lssvc_msgbuffer.h"
#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <sys/uio.h>

namespace lssvc::network {

class LSSTcpConnection;
using TcpConnectionPtr = std::shared_ptr<LSSTcpConnection>;
using CloseConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using TimeoutCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback =
    std::function<void(const TcpConnectionPtr &, LSSMsgBuffer &buffer)>;

struct BufferNode {
  BufferNode(void *buf, size_t s) : addr(buf), size(s) {}
  void *addr{nullptr};
  size_t size{0};
};

using BufferNodePtr = std::shared_ptr<BufferNode>;

struct TimeoutEntry;

class LSSTcpConnection : public LSSConnection {

public:
  /**
   * @brief construct a new LSSTcpConnection object
   * @param loop [in] EventLoop
   * @param sockfd [in] the incoming client's socket fd
   * @param localAddr [in] client's address
   * @param peerAddr [in] server's address
   */
  LSSTcpConnection(LSSEventLoop *loop, int sockfd,
                   const LSSInetAddress &localAddr,
                   const LSSInetAddress &peerAddr);

  ~LSSTcpConnection();

  /**
   * @brief set close callback for connection
   * @tparam Callback must be CloseConnectionCallback type
   * @param cb [in] the input function
   */
  template <typename Callback> void setCloseCallback(Callback &&cb) {
    close_cb_ = std::forward<Callback>(cb);
  }

  /**
   * @brief set message callback function
   * @tparam Callback must be MessageCallback type
   * @param cb [in] the input function
   */
  template <typename Callback> void setRecvMsgCallback(Callback &&cb) {
    message_cb_ = std::forward<Callback>(cb);
  }

  template <typename Callback> void setWriteCompleteCallback(Callback &&cb) {
    write_complete_cb_ = std::forward<Callback>(cb);
  }

  /**
   * @brief set timeout callback
   *
   * @param timeout [in] for timing-wheel, stores the seconds when this callback
   * should executed
   * @param cb [in] callback function
   */
  void setTimeoutCallback(int timeout, const TimeoutCallback &cb);
  void setTimeoutCallback(int timeout, TimeoutCallback &&cb);

  // @brief handle read events
  void onRead() override;

  // @brief handle close events
  void onClose() override;

  // @brief handle error events
  void onError(const std::string &msg) override;

  // @brief handle write events
  void onWrite() override;

  // @brief handle timeout events
  // @note we should close a TCP connection when time runs out
  void onTimeout();

  void enableCheckIdleTimeout(int32_t max_time);

  void send(std::list<BufferNodePtr> &list);
  void send(const char *buf, size_t size);

  // @brief force close this connection
  void forceClose() override;

private:
  // @brief extend the lifetime of a TCP connection
  // @note if a connection has been closed, this function does nothing
  void extendLife();

  void sendInLoop(std::list<BufferNodePtr> &list);
  void sendInLoop(const char *buf, size_t size);

  // operations on TcpConnection object are single-threaded, so use bool instead
  // of std::atomic<bool> is fine too
  std::atomic<bool> closed_{false}; // whether this connection is closed or not

  // Automately execute this function when connection is closed.
  CloseConnectionCallback close_cb_;

  LSSMsgBuffer message_buffer_;
  MessageCallback message_cb_;

  std::vector<struct iovec>
      io_vec_list_; // stores messages needed to be written
  WriteCompleteCallback write_complete_cb_; // callback after onWrite

  int32_t max_idle_time_{30}; // s, default 30s
  std::weak_ptr<TimeoutEntry> timeout_entry_;
};

struct TimeoutEntry {
  TimeoutEntry(const TcpConnectionPtr &c) : conn(c) {}
  ~TimeoutEntry() {
    auto ptr = conn.lock();
    if (ptr) {
      ptr->onTimeout();
    }
  }
  std::weak_ptr<LSSTcpConnection> conn;
};

}; // namespace lssvc::network

#endif
