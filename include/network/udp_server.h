#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

#include "base/lssvc_inetaddress.h"
#include "base/lssvc_socketopt.h"
#include "net/lssvc_eventloop.h"
#include "net/lssvc_udp_socket.h"

namespace lssvc::network {

class UdpServer : public LSSUdpSocket {
public:
  UdpServer(LSSEventLoop *loop, const LSSInetAddress &server);
  virtual ~UdpServer();

  // @brief start this udp server
  void start();

  // @brief stop this udp server
  void stop();

private:
  void open();
  LSSInetAddress server_;
};

} // namespace lssvc::network

#endif
