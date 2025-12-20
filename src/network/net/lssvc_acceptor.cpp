#include "network/net/lssvc_acceptor.h"
#include "network/base/lssvc_netlogger.h"
#include "network/base/lssvc_socketopt.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

using namespace lssvc::network;

LSSAcceptor::LSSAcceptor(LSSEventLoop *loop, const LSSInetAddress &addr)
    : LSSEvent(loop), addr_(addr)  {}

LSSAcceptor::~LSSAcceptor() {
  stop();
  if(socket_opt_ != nullptr) {
    delete socket_opt_;
    socket_opt_ = nullptr;
  }
}

void LSSAcceptor::setAcceptCallback(AcceptCallback &cb) {
  accept_cb_ = cb;
}

void LSSAcceptor::setAcceptCallback(AcceptCallback &&cb) {
  accept_cb_ = cb;
}

void LSSAcceptor::start() {
  loop_->enqueueTask([this](){
    open();
  });
}

void LSSAcceptor::stop() {
  loop_->delEvent(std::dynamic_pointer_cast<LSSAcceptor>(shared_from_this()));
}

void LSSAcceptor::onRead() {
  if(socket_opt_ == nullptr) {
    return;
  }
  while(true) {
    LSSInetAddress addr;
    int sock = socket_opt_->accept(&addr);
    if(sock > 0) {
      if(accept_cb_ != nullptr) {
        // execute accept callback
        accept_cb_(sock, addr);
      }
    } else {
      if(errno != EINTR && errno != EAGAIN) {
        // error occurs
        onClose();
      }
      break;
    }
  }
}

void LSSAcceptor::onError(const std::string &msg) {
  NETWORK_ERROR << "acceptor error "<< msg <<" (err " << errno << ").\r\n";
  onClose();
}

void LSSAcceptor::onClose() {
  stop();
  open();
}

void LSSAcceptor::open() {
  if(fd_ > 0) {
    ::close(fd_);
    fd_ = -1;
  }
  if(addr_.isIpv6()) {
    // ipv6
    fd_ = LSSocketOpt::createNonblockingTcpSocket(AF_INET6);
  } else {
    // ipv4
    fd_ = LSSocketOpt::createNonblockingTcpSocket(AF_INET);
  }
  if(fd_ < 0) {
    NETWORK_ERROR << "Failed to create tcp socket, (err" << errno << ")\r\n";
    exit(-1);
  }
  if(socket_opt_ != nullptr) {
    delete socket_opt_;
    socket_opt_ = nullptr;
  }
  loop_->addEvent(std::dynamic_pointer_cast<LSSAcceptor>(shared_from_this()));
  socket_opt_ = new LSSocketOpt(fd_, addr_.isIpv6());
  socket_opt_->setReuseAddr();
  socket_opt_->setReusePort();
  socket_opt_->bindAddress(addr_);
  socket_opt_->listen();
}
