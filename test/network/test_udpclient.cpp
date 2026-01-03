#include "network/base/lssvc_inetaddress.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/udp_client.h"

using namespace lssvc::utils;
using namespace lssvc::network;

LSSEventLoopThread eventloop_thread;

int main(int argc, char **argv) {
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();
  if (loop) {
    LSSInetAddress server("192.168.186.132:25678");
    std::shared_ptr<UdpClient> client =
        std::make_shared<UdpClient>(loop, server);
    client->setRecvMsgCallback(
        [](const LSSInetAddress &addr, LSSMsgBuffer &buf) {
          std::cout << "host:" << addr.toIpWithPort() << " msg:" << buf.peek()
                    << "\r\n";
          buf.retrieveAll();
        });

    client->setWriteCompleteCallback([](const UdpSocketPtr &conn) {
      if (conn) {
        std::cout << "host: " << conn->getPeerAddr().toIpWithPort()
                  << " write complete.\r\n";
      }
    });

    client->setConnectedCallback(
        [&client](const UdpSocketPtr &conn, bool connected) {
          if (connected) {
            client->send("hello world", strlen("hello world"));
          }
        });

    client->connect();
    while (1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  return 0;
}
