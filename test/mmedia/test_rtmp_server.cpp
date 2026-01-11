#include "mmedia/base/mmedia_logger.h"
#include "mmedia/rtmp/rtmp_handshake.h"
#include "mmedia/rtmp/rtmp_server.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "utils/lssvc_logger.h"

using namespace lssvc::mmedia;
using namespace lssvc::network;
using namespace lssvc::utils;

LSSEventLoopThread eventloop_thread;

int main(int argc, char **argv) {
  g_lsslogger->setLogLevel(kTrace);
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();
  if(loop) {
    LSSInetAddress listen("192.168.186.132:1935");
    RtmpServer server(loop, listen);
    server.start();
    while(1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  return 0;
}
