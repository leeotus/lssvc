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
using MessageCallback =
    std::function<void(const TcpConnectionPtr &, LSSMsgBuffer &buffer)>;

struct BufferNode {
  BufferNode(void *buf, size_t s) : addr(buf), size(s) {}
  void *addr{nullptr};
  size_t size{0};
};

using BufferNodePtr = std::shared_ptr<BufferNode>;

class LSSTcpConnection : public LSSConnection {

public:
  LSSTcpConnection(LSSEventLoop *loop, int sockfd,
                   const LSSInetAddress &localAddr, LSSInetAddress &peerAddr);
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

  template <typename Callback>
  void setWriteCompleteCallback(const Callback &&cb) {
    write_complete_cb_ = std::forward<Callback>(cb);
  }

  // @brief handle read events
  void onRead() override;

  // @brief handle close events
  void onClose() override;

  // @brief handle error events
  void onError(const std::string &msg) override;

  // @brief handle write events
  void onWrite() override;

  void send(std::list<BufferNodePtr> &list);
  void send(const char *buf, size_t size);

  // @brief force close this connection
  void forceClose() override;

private:
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
};

}; // namespace lssvc::network

#endif
