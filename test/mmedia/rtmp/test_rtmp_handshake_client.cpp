#include "mmedia/rtmp/rtmp_handshake.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/base/lssvc_netlogger.h"
#include "network/tcp_client.h"

#include <iostream>
#include <memory>

using namespace std;
using namespace lssvc::network;
using namespace lssvc::utils;
using namespace lssvc::mmedia;

LSSEventLoopThread eventloop_thread;
std::thread th;
using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;
const char *http_request =
    "GET / HTTP/1.0\r\nHost: 192.168.1.200\r\nAccept: */*\r\nContent-Type: "
    "text/plain\r\nContent-Length: 0\r\n\r\n";
const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: "
                            "text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, char **argv) {
  local_logger = new LSSLogger();
  local_logger->setLogLevel(kTrace);
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();

  if (loop) {
    LSSInetAddress server("192.168.186.132:1935");
    std::shared_ptr<TcpClient> client =
        std::make_shared<TcpClient>(loop, server);

    client->setRecvMsgCallback([](const TcpConnectionPtr &conn,
                                  LSSMsgBuffer &buf) {
      // handle server's incoming RTMP handshake packet
      RtmpHandShakePtr shake = conn->getContext<RtmpHandShake>(kNormalContext);
      shake->handShake(buf);
    });

    client->setCloseCallback([](const TcpConnectionPtr &conn) {
      if (conn) {
        NETWORK_INFO << "host:" << conn->getPeerAddr().toIpWithPort()
                     << " closed";
      }
    });

    client->setWriteCompleteCallback([](const TcpConnectionPtr &conn) {
      if (conn) {
        NETWORK_INFO << "host:" << conn->getPeerAddr().toIpWithPort()
                     << " write complete";
        RtmpHandShakePtr shake =
            conn->getContext<RtmpHandShake>(kNormalContext);
        shake->writeComplete();
      }
    });

    client->setConnectCallback([](const TcpConnectionPtr &conn,
                                  bool connected) {
      if (connected) {
        RtmpHandShakePtr shake = std::make_shared<RtmpHandShake>(conn, true);
        conn->setContext(kNormalContext, shake);
        shake->start();
      }
    });

    client->connect();
    while(1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  delete local_logger;
  return 0;
}
