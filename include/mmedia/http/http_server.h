#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "http_handler.h"
#include "network/net/lssvc_tcpconn.h"
#include "network/tcp_server.h"

namespace lssvc::mmedia {

class HttpServer : public network::TcpServer {
public:
  HttpServer(network::LSSEventLoop *loop, const network::LSSInetAddress &local,
             HttpHandler *handler = nullptr);
  ~HttpServer();

  // @brief start this http-server
  void start() override;

  // @brief stop this http-server
  void stop() override;

private:
  // @brief handle new http-connection
  void onNewConnection(const network::TcpConnectionPtr &conn);

  // @brief handle http-clients' disconnection
  void onDestroyed(const network::TcpConnectionPtr &conn);

  // @brief handle http-clients' http messages
  void onMessage(const network::TcpConnectionPtr &conn, network::LSSMsgBuffer &buf);

  // @brief handler aftering complete writing data to the http-clients
  void onWriteComplete(const network::ConnectionPtr &conn);

  void onActive(const network::ConnectionPtr &conn);

  HttpHandler *http_handler_{nullptr};
};

}   // namespace lssvc::mmedia

#endif
