#include "network/base/lssvc_inetaddress.h"
#include "network/net/lssvc_acceptor.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include <iostream>

using namespace lssvc::network;

LSSEventLoopThread eventloop_thread;

int main(int argc, char **argv) {
  eventloop_thread.run();
  LSSEventLoop *loop = eventloop_thread.loop();
  if(loop != nullptr) {
    LSSInetAddress server("192.168.186.132:25678");
    std::shared_ptr<LSSAcceptor> acceptor = std::make_shared<LSSAcceptor>(loop, server);
    acceptor->setAcceptCallback([](int fd, const LSSInetAddress &addr){
      std::cout << "host:" << addr.toIpWithPort() << "\r\n";
    });
    acceptor->start();
    while(true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  return 0;
}
