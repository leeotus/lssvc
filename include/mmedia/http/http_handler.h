#ifndef __HTTP_HANDLER_H__
#define __HTTP_HANDLER_H__

#include "mmedia/base/mmedia_handler.h"
#include "mmedia/base/packet.h"
#include <memory>

namespace lssvc::mmedia {

class HttpRequest;
using HttpRequestPtr = std::shared_ptr<HttpRequest>;

class HttpHandler : virtual public MMediaHandler {
public:
  // @brief callback after sending http message to the peer connection
  virtual void onSend(const network::TcpConnectionPtr &conn) = 0;

  // @brief callback when it is ready to send next http chunk
  virtual bool onSendNextChunk(const network::TcpConnectionPtr &conn) = 0;

  // @brief handle new request
  virtual void onRequest(const network::TcpConnectionPtr &conn,
                         const HttpRequestPtr &req, const PacketPtr &pkt) = 0;
};

}   // namespace lssvc::mmedia

#endif
