#include "mmedia/rtmp/rtmp_handshake.h"
#include "network/base/lssvc_inetaddress.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/tcp_server.h"

#include <iostream>
#include <memory>

using namespace std;
using namespace lssvc::network;
using namespace lssvc::mmedia;
using namespace lssvc::utils;

LSSEventLoopThread eventloop_thread;
std::thread th;
using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;
const char *http_response="HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, char **argv) {
  local_logger = new LSSLogger();
  local_logger->setLogLevel(kTrace);
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();

  if(loop) {
    LSSInetAddress listen("192.168.186.132:1935");
    TcpServer server(loop, listen);
    server.setMessageCallback([](const TcpConnectionPtr &conn, LSSMsgBuffer &buf){
      RtmpHandShakePtr shake = conn->getContext<RtmpHandShake>(kNormalContext);
      shake->handShake(buf);
    });
    server.setNewConnectionCallback([&loop](const TcpConnectionPtr &conn){
      // create RTMP handshake packet and store it in the context, send it through the connection
      RtmpHandShakePtr shake = std::make_shared<RtmpHandShake>(conn, false);
      conn->setContext(kNormalContext, shake);

      // start handshake
      shake->start();

      // wait for client's Rtmp packet
      conn->setWriteCompleteCallback([&loop](const TcpConnectionPtr &conn){
        NETWORK_INFO << "write complete host:" << conn->getPeerAddr().toIpWithPort();
        RtmpHandShakePtr shake = conn->getContext<RtmpHandShake>(kNormalContext);
        shake->writeComplete();
      });
    });

    server.start();
    while(1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    server.stop();
  }
  delete local_logger;
  return 0;
}
