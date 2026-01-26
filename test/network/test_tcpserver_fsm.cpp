#include "network/base/lssvc_inetaddress.h"
#include "network/base/lssvc_msgbuffer.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_event.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/tcp_server.h"
#include "test_context.h"
#include "utils/lssvc_logger.h"

#include <iostream>
#include <thread>

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace std;

using TestContextPtr = std::shared_ptr<TestContext>;

LSSEventLoopThread eventloop_thread;
const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: "
                            "text/html\r\nContent-Length: 0\r\n\r\n";

// test single thread's QPS:
// > ab -c 100 -n 500000 "http://192.168.186.132:25678/"
int main(int argc, char **argv) {
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();
  local_logger = new LSSLogger();

  if (loop) {
    LSSInetAddress listen_addr("192.168.186.132:25678");

    TcpServer server(loop, listen_addr);
    // set message callback
    server.setMessageCallback(
        [](const TcpConnectionPtr &con, LSSMsgBuffer &buf) {
          TestContextPtr context = con->getContext<TestContext>(kNormalContext);
          context->parseMessage(buf);
        });

    // set new connection callback
    server.setNewConnectionCallback([&loop](const TcpConnectionPtr &con) {
      TestContextPtr context = std::make_shared<TestContext>(con);
      context->setTestMessageCallback(
          [](const TcpConnectionPtr &con, const std::string &msg) {
            NETWORK_DEBUG << "message: " << msg;
          });
      con->setContext(kNormalContext, context);

      // set write complete callback
      con->setWriteCompleteCallback([&loop](const TcpConnectionPtr &con) {
        NETWORK_DEBUG << "write complete host:"
                      << con->getPeerAddr().toIpWithPort();
      });
    });

    server.start();
    while (1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  delete local_logger;
  return 0;
}
