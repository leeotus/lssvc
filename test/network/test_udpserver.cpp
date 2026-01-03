#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/udp_server.h"

#include <iostream>
#include <memory>

using namespace lssvc::network;
using namespace std;

LSSEventLoopThread event_loop_thread;

int main(int argc, char **argv) {
  event_loop_thread.run();
  LSSEventLoop *loop = event_loop_thread.loop();
  if (loop) {
    LSSInetAddress listen("192,168.186.132:25678");
    std::shared_ptr<UdpServer> server =
        std::make_shared<UdpServer>(loop, listen);
    server->setRecvMsgCallback(
        [&server](const LSSInetAddress &addr, LSSMsgBuffer &buf) {
          std::cout << "host:" << addr.toIpWithPort() << " msg:" << buf.peek()
                    << "\r\n";
          struct sockaddr_in6 sock_addr;
          addr.getSockAddr((struct sockaddr *)&sock_addr);
          server->send(buf.peek(), buf.readableBytes(),
                       (struct sockaddr *)&sock_addr, sizeof(sock_addr));
          buf.retrieveAll();
        });

    server->setCloseCallback([](const UdpSocketPtr &conn) {
      if (conn) {
        std::cout << "host:" << conn->getPeerAddr().toIpWithPort()
                  << " closed.\r\n";
      }
    });

    server->setWriteCompleteCallback([](const UdpSocketPtr &conn) {
      if (conn) {
        std::cout << "host:" << conn->getPeerAddr().toIpWithPort()
                  << " write complete.\r\n";
      }
    });

    server->start();

    while (1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  return 0;
}
