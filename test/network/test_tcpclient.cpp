#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/tcp_client.h"

#include <iostream>
#include <thread>

using namespace lssvc::network;
LSSEventLoopThread eventloop_thread;
std::thread th;
const char *http_request =
    "GET / HTTP/1.0\r\nHost: 192.168.186.132\r\nAccept: */*\r\nContent-Type: "
    "text/plain\r\nContent-Length: 0\r\n\r\n";
const char *http_response = "HTTP/1.0 200 OK\r\nServer: lssvc\r\nContent-Type: "
                            "text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, const char **agrv) {
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();

  if (loop) {
    LSSInetAddress server("192.168.186.132:25678");
    std::shared_ptr<TcpClient> client =
        std::make_shared<TcpClient>(loop, server);
    client->setRecvMsgCallback(
        [](const TcpConnectionPtr &con, LSSMsgBuffer &buf) {
          std::cout << "host:" << con->getPeerAddr().toIpWithPort()
                    << " msg:" << buf.peek() << std::endl;
          buf.retrieveAll();
        });
    client->setCloseCallback([](const TcpConnectionPtr &con) {
      if (con) {
        std::cout << "host:" << con->getPeerAddr().toIpWithPort() << " closed."
                  << std::endl;
      }
    });
    client->setWriteCompleteCallback([](const TcpConnectionPtr &con) {
      if (con) {
        std::cout << "host:" << con->getPeerAddr().toIpWithPort()
                  << " write complete. " << std::endl;
      }
    });
    client->setConnectCallback([](const TcpConnectionPtr &con, bool connected) {
      if (connected) {
        con->send(http_request, strlen(http_request));
      }
    });

    client->connect();

    while (1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  return 0;
}
