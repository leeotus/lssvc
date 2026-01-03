#include "network/udp_server.h"
#include "network/base/lssvc_socketopt.h"

using namespace lssvc::network;

UdpServer::UdpServer(LSSEventLoop *loop, const LSSInetAddress &server)
    : LSSUdpSocket(loop, -1, server, LSSInetAddress()), server_(server) {}

UdpServer::~UdpServer() { stop(); }

void UdpServer::start() {
  loop_->enqueueTask([this]() { open(); });
}

void UdpServer::stop() {
  loop_->enqueueTask([this]() {
    loop_->delEvent(std::dynamic_pointer_cast<UdpServer>(shared_from_this()));
    onClose();
  });
}

void UdpServer::open() {
  loop_->checkInLoopThread();
  fd_ = LSSocketOpt::createNonblockingUdpSocket(AF_INET);
  if (fd_ < 0) {
    onClose();
    return;
  }
  loop_->addEvent(std::dynamic_pointer_cast<UdpServer>(shared_from_this()));
  LSSocketOpt opt(fd_);
  opt.bindAddress(server_);
}
