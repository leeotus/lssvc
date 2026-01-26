#include "mmedia/rtmp/rtmp_client.h"
#include "mmedia/rtmp/rtmp_handler.h"
#include "mmedia/rtmp/rtmp_handshake.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/tcp_client.h"

#include <iostream>
#include <memory>

using namespace std;
using namespace lssvc::network;
using namespace lssvc::utils;
using namespace lssvc::mmedia;

LSSEventLoopThread eventloop_thread;

class RtmpHandlerImpl : public RtmpHandler {
public:
  void onNewConnection(const TcpConnectionPtr &conn) override {}
  void onConnectionDestroy(const TcpConnectionPtr &conn) override {}
  void onRecv(const TcpConnectionPtr &conn, const PacketPtr &data) override {
    std::cout << "recv type:" << data->getPacketType()
              << " size:" << data->getPacketSize() << std::endl;
  }
  void onRecv(const TcpConnectionPtr &conn, PacketPtr &&data) override {
    std::cout << "recv type:" << data->getPacketType()
              << " size:" << data->getPacketSize() << std::endl;
  }
  void onActive(const ConnectionPtr &conn) {}
  bool onPlay(const TcpConnectionPtr &conn, const std::string &session_name,
              const std::string &param) {
    return false;
  }
  virtual bool onPublish(const TcpConnectionPtr &conn,
                         const std::string &session_name,
                         const std::string &param) {
    return false;
  }
  virtual void onPublishPrepare(const TcpConnectionPtr &conn) {}
  virtual void onPause(const TcpConnectionPtr &conn, bool pause) {}
  virtual void onSeek(const TcpConnectionPtr &conn, double time) {}
};

int main(int argc, char **argv) {
  local_logger = new LSSLogger();
  local_logger->setLogLevel(kTrace);
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();

  if (loop) {
    RtmpClient client(loop, new RtmpHandlerImpl());
    // TODO: server haven't implement stream pushing
    client.play("rtmp://localhost/ucloud/test");
    while(1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  delete local_logger;
  return 0;
}
