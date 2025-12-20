#ifndef __LSSVC_ACCEPTOR_H__
#define __LSSVC_ACCEPTOR_H__

#include "network/base/lssvc_inetaddress.h"
#include "lssvc_event.h"
#include "lssvc_eventloop.h"

#include <functional>
#include <memory>

namespace lssvc::network {

using AcceptCallback = std::function<void(int sock_fd, const LSSInetAddress &addr)>;

class LSSocketOpt;
class LSSAcceptor : public LSSEvent {
public:
  /**
   * @brief construct an acceptor object
   * @param loop [in] which eventloop this acceptor in
   * @param addr [in] the binding address
   */
  LSSAcceptor(LSSEventLoop *loop, const LSSInetAddress &addr);
  ~LSSAcceptor();

  void setAcceptCallback(AcceptCallback &cb);

  void setAcceptCallback(AcceptCallback &&cb);

  /// @brief start the (server) acceptor
  void start();

  /// @brief stop the (server) accepter
  void stop();

  /// @brief handle read events
  void onRead() override;

  /// @brief handle error events
  void onError(const std::string &msg) override;

  /// @brief handle close events
  void onClose() override;

private:
  /// @brief open file description of this (server)socket
  void open();

  LSSInetAddress addr_;
  AcceptCallback accept_cb_;
  LSSocketOpt *socket_opt_{nullptr};
};

}   // lssvc::network

#endif
