#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "mmedia/base/packet.h"
#include "mmedia/http/http_handler.h"
#include "mmedia/http/http_request.h"
#include "network/base/lssvc_inetaddress.h"
#include "network/net/lssvc_eventloop.h"
#include "network/tcp_client.h"
#include <functional>
#include <memory>
#include <string>

namespace lssvc::mmedia {

class HttpClient {
public:
  HttpClient(network::LSSEventLoop *loop, HttpHandler *handler);
  ~HttpClient();

  // @brief set close callback
  template <typename Callback>
  void setCloseCallback(Callback &&cb) {
    close_cb_ = std::forward<Callback>(cb);
  }

  /**
   * @brief http GET request
   * @param url [in] http-server's address
   */
  void get(const std::string &url);

  /**
   * @brief http POST request
   * @param url [in] http-server's address
   * @param pkt [in] data which needs to be sent
   */
  void post(const std::string &url, const PacketPtr &pkt);

private:
  void onWriteComplete(const network::TcpConnectionPtr &conn);
  void onConnection(const network::TcpConnectionPtr &conn, bool connected);
  void onMessage(const network::TcpConnectionPtr &conn, network::LSSMsgBuffer &buf);
  bool parseUrl(const std::string &url);
  void createTcpClient();

  network::LSSEventLoop *loop_{nullptr};
  network::LSSInetAddress addr_; // server address
  HttpHandler *handler_{nullptr};
  network::TcpClientPtr tcp_client_;
  std::string url_;
  bool is_post_{false}; // post request
  network::CloseConnectionCallback close_cb_;
  HttpRequestPtr request_;
  PacketPtr out_packet_; // store data if is_post is true
};

}   // namespace lssvc::mmedia

#endif
