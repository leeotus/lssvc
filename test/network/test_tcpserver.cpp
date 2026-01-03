#include "network/base/lssvc_inetaddress.h"
#include "network/base/lssvc_msgbuffer.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_event.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/tcp_server.h"
#include "utils/lssvc_logger.h"

#include <iostream>
#include <thread>

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace std;

LSSEventLoopThread eventloop_thread;
const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: "
                            "text/html\r\nContent-Length: 0\r\n\r\n";

// test single thread's QPS:
// > ab -c 100 -n 500000 "http://192.168.186.132:25678/"
int main(int argc, char **argv) {
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();
  g_lsslogger->setLogLevel(kError);

  if (loop) {
    LSSInetAddress listen_addr("192.168.186.132:25678");
    TcpServer server(loop, listen_addr);
    server.setMessageCallback(
        [](const TcpConnectionPtr &con, LSSMsgBuffer &buf) {
          std::cout << "host:" << con->getPeerAddr().toIpWithPort()
                    << " msg:" << buf.peek() << "\r\n";
          buf.retrieveAll();
          con->send(http_response, strlen(http_response));
        });
    server.setNewConnectionCallback([&loop](const TcpConnectionPtr &con) {
      con->setWriteCompleteCallback([&loop](const TcpConnectionPtr &con) {
        std::cout << "write complete host:" << con->getPeerAddr().toIpWithPort()
                  << "\r\n";
        con->forceClose();
      });
    });
    server.start();
    while (1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  return 0;
}
