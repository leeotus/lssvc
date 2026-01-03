#include "network/udp_client.h"
#include "network/base/lssvc_netlogger.h"
#include "network/base/lssvc_socketopt.h"

using namespace lssvc::network;

UdpClient::UdpClient(LSSEventLoop *loop, const LSSInetAddress &server)
    : LSSUdpSocket(loop, -1, LSSInetAddress(), server), server_addr_(server) {}

UdpClient::~UdpClient() {}

void UdpClient::connect() {
  loop_->enqueueTask([this]() { connectInLoop(); });
}

void UdpClient::connectInLoop() {
  loop_->checkInLoopThread();
  fd_ = LSSocketOpt::createNonblockingUdpSocket(AF_INET);
  if (fd_ < 0) {
    NETWORK_ERROR << "Failed to create a nonblocking udp client.";
    onClose(); // in fact, this will not execute
    return;
  }
  connected_ = true;
  // add to the EventLoop
  loop_->addEvent(std::dynamic_pointer_cast<UdpClient>(shared_from_this()));
  LSSocketOpt opt(fd_); // use ipv4 default
  opt.connect(server_addr_);
  server_addr_.getSockAddr((struct sockaddr *)&sock_addr_);
  if (connected_cb_) {
    connected_cb_(std::dynamic_pointer_cast<LSSUdpSocket>(shared_from_this()),
                  true);
  }
}

void UdpClient::onClose() {
  if (connected_) {
    connected_ = false;
    loop_->delEvent(std::dynamic_pointer_cast<UdpClient>(shared_from_this()));
    LSSUdpSocket::onClose();
  }
}

void UdpClient::send(std::list<UdpBufferNodePtr> &list) {
  if (connected_) {
    LSSUdpSocket::send(list);
  }
}
void UdpClient::send(const char *buf, size_t size) {
  if (connected_) {
    LSSUdpSocket::send(buf, size, (struct sockaddr *)&sock_addr_, sock_len_);
  }
}
