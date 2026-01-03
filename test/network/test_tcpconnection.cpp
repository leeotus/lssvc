#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/net/lssvc_tcpconn.h"

#include <iostream>
#include <memory>
#include <vector>

using namespace std;
using namespace lssvc::network;

const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: "
                            "text/html\r\nContent-Length: 0\r\n\r\n";

LSSEventLoopThread eventloop_thread;

int main(int argc, char **argv) {
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();
  if (loop) {
    std::vector<TcpConnectionPtr> list; // stores incoming tcp connections
    LSSInetAddress server("192.168.186.132:25678"); // server's address and ip
    std::shared_ptr<LSSAcceptor> acceptor =
        std::make_shared<LSSAcceptor>(loop, server);
    acceptor->setAcceptCallback(
        [&loop, &server, &list](int fd, const LSSInetAddress &addr) {
          std::cout << "host: " << addr.toIpWithPort() << "\r\n";
          TcpConnectionPtr conn =
              std::make_shared<LSSTcpConnection>(loop, fd, server, addr);

          // set incoming tcp connections' RecvMsgCallback,
          // WriteCompleteCallback and Timeoutallback
          conn->setRecvMsgCallback(
              [](const TcpConnectionPtr &con, LSSMsgBuffer &buf) {
                std::cout << "recv msg:" << buf.peek() << "\r\n";
                buf.retrieveAll();
                con->send(http_response, strlen(http_response));
              });
          conn->setWriteCompleteCallback([&loop](const TcpConnectionPtr &con) {
            std::cout << "write complete host: "
                      << con->getPeerAddr().toIpWithPort() << "\r\n";
            loop->delEvent(con);
            con->forceClose();
          });
          conn->setTimeoutCallback(3, [](const TcpConnectionPtr &con) {
            NETWORK_DEBUG << "time runs out!\r\n";
          });
          list.push_back(conn);
          loop->addEvent(conn);
        });
    acceptor->start();
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  return 0;
}
