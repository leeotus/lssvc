#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include "base/lssvc_inetaddress.h"
#include "net/lssvc_eventloop.h"
#include "net/lssvc_tcpconn.h"

#include <functional>
#include <list>

namespace lssvc {
namespace network {

enum {
  kTcpConnStatusInit = 0,
  kTcpConnStatusConnecting,
  kTcpConnStatusConnected,
  kTcpConnStatusDisconnected,
};

using ConnectionCallback = std::function<void(const TcpConnectionPtr &, bool)>;

class TcpClient : public LSSTcpConnection {
public:
  /**
   * @brief construct a tcp client object
   * @param loop [in] EventLoop object
   * @param server [in] server address
   */
  TcpClient(LSSEventLoop *loop, const LSSInetAddress &server);
  virtual ~TcpClient();

  // @brief connect to the server
  void connect();

  // @brief set connected callback function
  template <typename Callback> void setConnectCallback(Callback &&cb) {
    connected_cb_ = std::forward<Callback>(cb);
  }

  /**
   * @brief check and update client's status before calling
   * TcpConnection::onRead() function
   */
  void onRead() override;

  /**
   * @brief check and update client's status before calling
   * TcpConnection::onWrite() function
   */
  void onWrite() override;

  /**
   * @brief check the client' status, if it's in connecting or connected
   * status, delete this client in the EventLoop. update the status in the end.
   */
  void onClose() override;

  // @brief check the connection status before sending data
  void send(std::list<BufferNodePtr> &list);
  void send(const char *buf, size_t size);

private:
  bool checkError();

  // @brief update tcp connection status
  void updateConnectionStatus();

  void connectInLoop();

  LSSInetAddress server_addr_;
  int status_{kTcpConnStatusInit};
  ConnectionCallback connected_cb_;
};

} // namespace network
} // namespace lssvc

#endif
