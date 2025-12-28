#include "network/net/lssvc_connection.h"

using namespace lssvc::network;

LSSConnection::LSSConnection(LSSEventLoop *loop, int fd,
                             const LSSInetAddress &localAddr,
                             const LSSInetAddress &peerAddr)
    : LSSEvent(loop, fd), local_addr_(localAddr), peer_addr_(peerAddr) {}

void LSSConnection::setLocalAddr(const LSSInetAddress &local) {
  local_addr_ = local;
}

void LSSConnection::setPeerAddr(const LSSInetAddress &peer) {
  peer_addr_ = peer;
}

const LSSInetAddress &LSSConnection::getLocalAddr() const {
  return local_addr_;
}

const LSSInetAddress &LSSConnection::getPeerAddr() const { return peer_addr_; }

void LSSConnection::setContext(int type, const std::shared_ptr<void> &context) {
  contexts_[type] = context;
}

void LSSConnection::clearContext(int type) { contexts_[type].reset(); }

void LSSConnection::clearContext() { contexts_.clear(); }

void LSSConnection::active() {
  // ensure that all operations related to the connection status are
  // performed in the same thread.
  if (!active_.load()) {
    loop_->enqueueTask([this]() {
      active_.store(true);
      if (active_cb_) {
        active_cb_(
            std::dynamic_pointer_cast<LSSConnection>(shared_from_this()));
      }
    });
  }
}

void LSSConnection::deActive() { active_.store(false); }
