#ifndef __RTMP_CLIENT_H__
#define __RTMP_CLIENT_H__

#include "network/tcp_client.h"
#include "network/base/lssvc_inetaddress.h"
#include "network/net/lssvc_eventloop.h"
#include "rtmp_handler.h"

#include <functional>
#include <memory>
#include <string>

namespace lssvc::mmedia {

class RtmpClient {
public:
  RtmpClient(network::LSSEventLoop *loop, RtmpHandler *handler);
  ~RtmpClient();

  // @brief set close callback
  template <typename Callback>
  void setCloseCallback(Callback &&cb) {
    close_cb_ = std::forward<Callback>(cb);
  }

  // @brief stream pulling
  void play(const std::string &url);

  // @brief stream pushing
  void publish(const std::string &url);

  void send(PacketPtr &&data);

private:
  void onWriteComplete(const network::TcpConnectionPtr &conn);
  void onConnection(const network::TcpConnectionPtr &conn, bool connected);
  void onMessage(const network::TcpConnectionPtr &conn, network::LSSMsgBuffer &buf);
  bool parseUrl(const std::string &url);

  void createTcpClient();

  network::LSSEventLoop *loop_{nullptr};
  network::LSSInetAddress addr_;
  RtmpHandler *handler_{nullptr};
  network::TcpClientPtr tcp_client_;
  std::string url_;
  bool is_player_{false};

  network::CloseConnectionCallback close_cb_;
};

} // namespace lssvc::mmedia

#endif
