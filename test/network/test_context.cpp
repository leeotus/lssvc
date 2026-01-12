#include "test_context.h"
#include "network/base/lssvc_netlogger.h"

using namespace lssvc::network;

TestContext::TestContext(const TcpConnectionPtr &conn) : connection_(conn) {}

int TestContext::parseMessage(LSSMsgBuffer &buf) {
  // if there exists data
  while (buf.readableBytes() > 0) {
    // switch states and do the corresponding things
    if (state_ == kTestContextHeader) {

      // assume the header has 4 bytes
      if (buf.readableBytes() >= 4) {
        // assume the header stores the length of the body
        message_length_ = buf.readInt32();
        NETWORK_DEBUG << "received " << message_length_ << " header";

        // switch state
        state_ = kTestContextBody;
        continue;

      } else {
        // failed to parse the header
        // require more data
        return 1;
      }
    } else if (state_ == kTestContextBody) {
      if (buf.readableBytes() >= message_length_) {
        // not only one message
        std::string tmp;
        tmp.assign(buf.peek(), message_length_);
        message_body_.append(tmp);
        buf.retrieve(message_length_); // clear
        message_length_ = 0;
        if (cb_) {
          cb_(connection_, message_body_);
          message_body_.clear();
        }

        // switch state
        state_ = kTestContextHeader;
      }
      // TODO: wait for other messages?
    }
  }
  return 0;
}
