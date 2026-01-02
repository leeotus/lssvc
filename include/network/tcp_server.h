#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "network/base/lssvc_inetaddress.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_tcpconn.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace lssvc::network {

using NewConnnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using DestroyConnectionCallback = std::function<void(const TcpConnectionPtr &)>;

class TcpServer {
public:
  TcpServer(LSSEventLoop *loop, const LSSInetAddress &addr);
  virtual ~TcpServer();

  // @brief start this server
  virtual void start();

  // @brief stop this server
  virtual void stop();

  template <typename Callback> void setNewConnectionCallback(Callback &&cb) {
    new_connection_cb_ = std::forward<Callback>(cb);
  }

  template <typename Callback>
  void setDestroyConnectionCallback(Callback &&cb) {
    destroy_connection_cb_ = std::forward<Callback>(cb);
  }

  template <typename Callback> void setActiveCallback(Callback &&cb) {
    active_cb_ = std::forward<Callback>(cb);
  }

  template <typename Callback> void setWriteCompleteCallback(Callback &&cb) {
    write_complete_cb_ = std::forward<Callback>(cb);
  }

  template <typename Callback> void setMessageCallback(Callback &&cb) {
    messsage_cb_ = std::forward<Callback>(cb);
  }

  /**
   * @brief handle incoming client's connection request
   *
   * @param fd [in] client's fd
   * @param addr [in] client's address
   */
  void onAccept(int fd, const LSSInetAddress &addr);

  /**
   * @brief connected clients' close event callback
   * @param con [in] pointer to the client
   */
  void onConnectionClose(const TcpConnectionPtr &con);

private:
  LSSEventLoop *loop_{nullptr};
  std::shared_ptr<LSSAcceptor> acceptor_;
  LSSInetAddress addr_;

  NewConnnectionCallback new_connection_cb_;
  std::unordered_set<TcpConnectionPtr> connections_;

  MessageCallback messsage_cb_;
  ActiveCallback active_cb_;
  WriteCompleteCallback write_complete_cb_;
  DestroyConnectionCallback destroy_connection_cb_;
};

} // namespace lssvc::network

#endif
