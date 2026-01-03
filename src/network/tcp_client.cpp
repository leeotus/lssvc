#include "network/tcp_client.h"
#include "network/base/lssvc_netlogger.h"
#include "network/base/lssvc_socketopt.h"
#include "network/net/lssvc_tcpconn.h"

#include <sys/socket.h>

namespace lssvc::network {

TcpClient::TcpClient(LSSEventLoop *loop, const LSSInetAddress &server)
    : LSSTcpConnection(loop, -1, LSSInetAddress(), server),
      server_addr_(server) {}

TcpClient::~TcpClient() { onClose(); }

void TcpClient::connect() {
  loop_->enqueueTask([this]() { connectInLoop(); });
}

void TcpClient::connectInLoop() {
  loop_->checkInLoopThread();
  fd_ = LSSocketOpt::createNonblockingTcpSocket(AF_INET);
  if (fd_ < 0) {
    // failed to create a tcp socket
    onClose();
    return;
  }
  status_ = kTcpConnStatusConnecting;

  // add this tcp client into the EventLoop
  loop_->addEvent(std::dynamic_pointer_cast<TcpClient>(shared_from_this()));
  enableWriting(true);
  enableCheckIdleTimeout(3);

  // connect to the server
  LSSocketOpt opt(fd_);
  // @todo dns
  int ret = opt.connect(server_addr_);
  if (ret == 0) {
    updateConnectionStatus();
    return;
  } else if (ret == -1) {
    if (errno != EINPROGRESS) {
      // not in the progress of conecting
      NETWORK_ERROR << "connect to server:" << server_addr_.toIpWithPort()
                    << " error:" << errno;
      onClose();
      return;
    }
  }
}

void TcpClient::updateConnectionStatus() {
  status_ = kTcpConnStatusConnected;
  if (connected_cb_) {
    connected_cb_(std::dynamic_pointer_cast<TcpClient>(shared_from_this()),
                  true);
  }
}

bool TcpClient::checkError() {
  int err{0};
  socklen_t len = sizeof(err);
  ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
  return err != 0;
}

void TcpClient::onRead() {
  if (status_ == kTcpConnStatusConnecting) {
    if (checkError()) {
      NETWORK_ERROR << "connect to server:" << server_addr_.toIpWithPort()
                    << " error:" << errno << "\r\n";
      // failed to connect to the server
      onClose();
      return;
    }
    updateConnectionStatus();
    return;
  } else if (status_ == kTcpConnStatusConnected) {
    LSSTcpConnection::onRead();
  }
}

void TcpClient::onWrite() {
  if (status_ == kTcpConnStatusConnecting) {
    if (checkError()) {
      NETWORK_ERROR << "connect to server:" << server_addr_.toIpWithPort()
                    << " error:" << errno << "\r\n";
      // failed to connect to the server
      onClose();
      return;
    }
    updateConnectionStatus();
    return;
  } else if (status_ == kTcpConnStatusConnected) {
    LSSTcpConnection::onWrite();
  }
}

void TcpClient::onClose() {
  if (status_ == kTcpConnStatusConnecting ||
      status_ == kTcpConnStatusConnected) {
    loop_->delEvent(std::dynamic_pointer_cast<TcpClient>(shared_from_this()));
  }
  status_ = kTcpConnStatusDisconnected;
  LSSTcpConnection::onClose();
}

void TcpClient::send(std::list<BufferNodePtr> &list) {
  if (status_ == kTcpConnStatusConnected) {
    LSSTcpConnection::send(list);
  }
}

void TcpClient::send(const char *buf, size_t size) {
  if (status_ == kTcpConnStatusConnected) {
    LSSTcpConnection::send(buf, size);
  }
}

} // namespace lssvc::network
