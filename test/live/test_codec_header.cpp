#include "live/base/codec_utils.h"
#include "live/codec_header.h"
#include "mmedia/rtmp/rtmp_client.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/tcp_client.h"
#include "utils/lssvc_logstream.h"
#include <iostream>

using namespace std;
using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::mmedia;
using namespace lssvc::live;

LSSEventLoopThread eventloop_thread;
std::thread th;
CodecHeader codec_header;

class RtmpHandlerImpl : public RtmpHandler {
public:
  void onNewConnection(const TcpConnectionPtr &conn) override {}
  void onConnectionDestroy(const TcpConnectionPtr &conn) override {}
  void onRecv(const TcpConnectionPtr &conn, const PacketPtr &data) override {
    // std::cout << "recv type:" << data->PacketType() << " size:" <<
    // data->PacketSize() << std::endl;
    if (CodecUtils::isCodecHeader(data)) {
      codec_header.parseCodecHeader(data);
    }
  }
  void onRecv(const TcpConnectionPtr &conn, PacketPtr &&data) override {
    // std::cout << "recv type:" << data->PacketType() << " size:" <<
    // data->PacketSize() << std::endl;
    if (CodecUtils::isCodecHeader(data)) {
      codec_header.parseCodecHeader(data);
    }
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
  virtual void onPause(const TcpConnectionPtr &conn, bool pause) {}
  virtual void onSeek(const TcpConnectionPtr &conn, double time) {}
  virtual void onPublishPrepare(const TcpConnectionPtr &conn) {}
};

int main(int argc, char **argv) {
  local_logger = new LSSLogger();
  local_logger->setLogLevel(kTrace);

  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();
  if(loop) {
    RtmpClient client(loop, new RtmpHandlerImpl());
    client.play("rtmp://192.168.186.132/live/test");
    while(1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  delete local_logger;
  return 0;
}
