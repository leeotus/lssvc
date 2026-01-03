#ifndef __UDP_CLIENT_H__
#define __UDP_CLIENT_H__

#include "network/net/lssvc_udp_socket.h"
#include <functional>
#include <list>
#include <string>

namespace lssvc::network {

using ConnectedCallback = std::function<void(const UdpSocketPtr &, bool)>;

class UdpClient : public LSSUdpSocket {
public:
  UdpClient(LSSEventLoop *loop, const LSSInetAddress &server);
  virtual ~UdpClient();

  template <typename Callback> void setConnectedCallback(Callback &&cb) {
    connected_cb_ = std::forward<Callback>(cb);
  }

  void connect();

  // @brief check the connection situation before sending
  void send(std::list<UdpBufferNodePtr> &list);
  void send(const char *buf, size_t size);

  void onClose() override;

private:
  void connectInLoop();

  bool connected_{false}; // connected or not
  ConnectedCallback connected_cb_;
  LSSInetAddress server_addr_;
  struct sockaddr_in6 sock_addr_;
  socklen_t sock_len_{sizeof(struct sockaddr_in6)};
};

} // namespace lssvc::network

#endif
