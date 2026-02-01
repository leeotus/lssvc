#include "mmedia/base/mmedia_logger.h"
#include "mmedia/http/http_client.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/tcp_client.h"

#include <iostream>
#include <thread>

using namespace lssvc::network;
using namespace lssvc::mmedia;

LSSEventLoopThread eventloop_thread;
std::thread th;

class HttpHandlerImpl : public HttpHandler {
public:
  void onNewConnection(const TcpConnectionPtr &conn) override {}
  void onConnectionDestroy(const TcpConnectionPtr &conn) override {}
  void onRecv(const TcpConnectionPtr &conn, const PacketPtr &data) override {
    std::cout << "receive type:" << data->getPacketType()
               << " size:" << data->getPacketSize();
  }
  void onRecv(const TcpConnectionPtr &conn, PacketPtr &&data) override {
    std::cout << "receive type:" << data->getPacketType()
               << " size:" << data->getPacketSize();
  }
  void onActive(const ConnectionPtr &conn) {}
  void onSend(const TcpConnectionPtr &conn) {}
  bool onSendNextChunk(const TcpConnectionPtr &conn) {
    return false;
  }
  void onRequest(const TcpConnectionPtr &conn, const HttpRequestPtr &req, const PacketPtr &pkt) {
    if(!req->isRequest()) {
      std::cout << "code:" << req->getStatusCode();
      if(pkt) {
        std::cout << "body\r\n" << pkt->data();
      }
    }
  }
};

int main(int argc, char **argv) {
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();
  if(loop) {
    HttpClient client(loop, new HttpHandlerImpl());
    client.get("http://bilibili.com");
    while(1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  return 0;
}
