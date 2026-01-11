#ifndef __RTMP_SERVER_H__
#define __RTMP_SERVER_H__

#include "network/base/lssvc_inetaddress.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_tcpconn.h"
#include "network/tcp_server.h"
#include "rtmp_handler.h"

namespace lssvc::mmedia {

class RtmpServer : public network::TcpServer {
public:
  RtmpServer(network::LSSEventLoop *loop, network::LSSInetAddress &local,
             RtmpHandler *handler = nullptr);
  ~RtmpServer();

  // @brief start RTMP server
  void start() override;

  // @brief stop RTMP server
  void stop() override;

private:
  void onNewConnection(const network::TcpConnectionPtr &conn);
  void onDestroyed(const network::TcpConnectionPtr &conn);
  void onMessage(const network::TcpConnectionPtr &conn, network::LSSMsgBuffer &buf);
  void onWriteComplete(const network::ConnectionPtr &conn);
  void onActive(const network::ConnectionPtr &conn);
  RtmpHandler *rtmp_handler_{nullptr};
};

} // namespace lssvc::mmedia

#endif
