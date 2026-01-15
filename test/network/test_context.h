#ifndef __TESTCONTEXT_H__
#define __TESTCONTEXT_H__

#include "network/net/lssvc_tcpconn.h"
#include <functional>
#include <memory>
#include <queue>
#include <string>

namespace lssvc::network {

// TcpConnectionPtr: to indicate the address, string: to print out the message
using TestMessageCallback =
    std::function<void(const TcpConnectionPtr &, const std::string &)>;

class TestContext {
  enum {
    kTestContextHeader, // parsing header
    kTestContextBody,   // parsing body
  };

public:
  TestContext(const TcpConnectionPtr &conn);
  ~TestContext() = default;

  /**
   * @brief parse the incoming message
   * @param buf [in] stores the message
   * @return int return 0 if success
   * @note using FSM(finite-state machine)
   */
  int parseMessage(LSSMsgBuffer &buf);

  template <typename Callback> void setTestMessageCallback(Callback &&cb) {
    cb_ = std::forward<Callback>(cb);
  }

private:
  TcpConnectionPtr connection_;
  int state_{kTestContextHeader};
  int32_t message_length_{0}; // incoming message's length
  std::string message_body_;

  TestMessageCallback cb_; // print out messages after parsing
};

}; // namespace lssvc::network

#endif
