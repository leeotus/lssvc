#ifndef __LSSVC_CONNECTION_H__
#define __LSSVC_CONNECTION_H__

#include "lssvc_event.h"
#include "lssvc_eventloop.h"
#include "network/base/lssvc_inetaddress.h"

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

namespace lssvc::network {

enum {
  kNormalContext = 0,
  kRtmpContext,
  kHttpContext,
  kUserContext,
  kFlvContext,
};

using ContextPtr = std::shared_ptr<void>;
class LSSConnection;
using ConectionPtr = std::shared_ptr<LSSConnection>;
using ActiveCallback = std::function<void(const ConectionPtr &)>;

class LSSConnection : public LSSEvent {
public:
  LSSConnection(LSSEventLoop *loop, int fd, const LSSInetAddress &localAddr,
                const LSSInetAddress &peerAddr);
  virtual ~LSSConnection() = default;

  // setter
  void setLocalAddr(const LSSInetAddress &local);
  void setPeerAddr(const LSSInetAddress &peer);

  // getter
  const LSSInetAddress &getLocalAddr() const;
  const LSSInetAddress &getPeerAddr() const;

  void setContext(int type, const std::shared_ptr<void> &context);

  template <typename CtxPtr> void setContext(int type, CtxPtr &&context) {
    contexts_[type] = std::forward<CtxPtr>(context);
  }

  template <typename T> std::shared_ptr<T> getContext(int type) const {
    // operations to connection object are proceeded in a single thread, so no
    // need mutex
    auto it = contexts_.find(type);
    if (it != contexts_.end()) {
      return std::dynamic_pointer_cast<T>(it->second);
    }
    return std::shared_ptr<T>();
  }

  void clearContext(int type);

  void clearContext();

  template <typename Callback> void setActiveCallback(Callback &&cb) {
    active_cb_ = std::forward<Callback>(cb);
  }

  void active();

  void deActive();

  // tcp and udp's close functions are different, therefore, make it virtual
  virtual void forceClose() = 0;

private:
  // stores different types of context data
  std::unordered_map<int, ContextPtr> contexts_;

  // automatically execute avtive callback when the conection transitions from
  // an inactive state to an active one, (for example, initialize connection
  // resources, record avtivity logs ...)
  ActiveCallback active_cb_;

  LSSInetAddress local_addr_; // this connection's address
  LSSInetAddress peer_addr_;  // server's address
  std::atomic<bool> active_{false};
};

} // namespace lssvc::network

#endif
