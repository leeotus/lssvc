#include "network/tcp_server.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_acceptor.h"

#include <memory>

using namespace lssvc::network;

TcpServer::TcpServer(LSSEventLoop *loop, const LSSInetAddress &addr)
    : loop_(loop), addr_(addr) {
  acceptor_ = std::make_shared<LSSAcceptor>(loop, addr);
}

TcpServer::~TcpServer() {}

void TcpServer::onAccept(int fd, const LSSInetAddress &addr) {
  NETWORK_TRACE << "new connection fd:" << fd
                << ", host:" << addr.toIpWithPort();
  TcpConnectionPtr conn =
      std::make_shared<LSSTcpConnection>(loop_, fd, addr_, addr);
  conn->setCloseCallback(
      std::bind(&TcpServer::onConnectionClose, this, std::placeholders::_1));
  if (write_complete_cb_) {
    conn->setWriteCompleteCallback(write_complete_cb_);
  }
  if (active_cb_) {
    conn->setActiveCallback(active_cb_);
  }
  conn->setRecvMsgCallback(messsage_cb_);
  connections_.insert(conn);
  loop_->addEvent(conn);

  // TODO: pass from the json configuration file
  conn->enableCheckIdleTimeout(30); // 30s
  if (new_connection_cb_) {
    new_connection_cb_(conn);
  }
}

void TcpServer::onConnectionClose(const TcpConnectionPtr &conn) {
  NETWORK_TRACE << "host:" << conn->getPeerAddr().toIpWithPort() << " closed.";
  loop_->checkInLoopThread();
  connections_.erase(conn);
  loop_->delEvent(conn);
  if (destroy_connection_cb_) {
    destroy_connection_cb_(conn);
  }
}

void TcpServer::start() {
  acceptor_->setAcceptCallback(std::bind(&TcpServer::onAccept, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2));
  acceptor_->start();
}

void TcpServer::stop() { acceptor_->stop(); }
